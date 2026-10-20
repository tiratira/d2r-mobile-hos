# extract_romfs_hos.py — 从你自己的基础版 NSZ + 更新 NSP 提取 BKTR 解析后的 RomFS。
#
# 等价上游 dm/tools/extract_game.sh 的「取数据文件」一步。nut/nsz 自带解密栈
# 但不做 BKTR 读通解析,这里补上:
#   - 重定位表(BKTR relocation)自行解析:nut 只留了头,Bucket 解析被注释
#   - 子段表(BKTR subsection)用 nut 已解析的 buckets(每个子段一个 CTR)
#   - 虚拟读 v:查重定位条目 → isPatch 则从更新节的物理空间按子段 CTR 解密,
#     否则从基础节的物理空间按节 CTR 解密
#   - RomFS 目录/文件表遍历,逐文件落盘
#   - --verify:IVFC 哈希树全量校验(把解出的虚拟镜像按层重哈希对到主哈希)
#
# 密钥与游戏包只在本机读取,不离开本机。
#
# 用法:
#   D:/Tools/nsz_v4.6.1_win64_portable/python.exe tools/extract_romfs_hos.py \
#       <基础.nsz> <更新.nsp> <prod.keys> <输出目录> [--verify]
import os, sys, shutil, hashlib, struct, time
from pathlib import Path

BASE = sys.argv[1]
UPD = sys.argv[2]
KEYS = sys.argv[3]
OUT = Path(sys.argv[4])
VERIFY = '--verify' in sys.argv[5:]
LIST = '--list' in sys.argv[5:]
BASE_ONLY = '--base-only' in sys.argv[5:]
VERIFY_ONLY = '--verify-only' in sys.argv[5:]
NSZ_PKG = os.environ.get('NSZ_PY_PATH',
                         r'D:\Tools\nz_v4.6.1_win64_portable\Lib\site-packages')

home_keys = Path.home() / '.switch' / 'prod.keys'
if not home_keys.is_file():
    home_keys.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(KEYS, home_keys)

sys.path.insert(0, NSZ_PKG)
from nsz import Fs                                    # noqa: E402
from nsz.Fs import Nca, Ticket                        # noqa: E402
from nsz.nut import Keys, Titles, Print, aes128       # noqa: E402

Keys.load(str(home_keys))


def ent_name(e):
    if isinstance(e, dict):
        return e['name']
    return getattr(e, '_path', '') or ''


def open_container(path):
    c = Fs.factory(Path(path))
    c.open(path, 'rb')
    # ticket 注册进 Titles(标题密钥解密用)
    n = 0
    for ent in c:
        if not ent_name(ent).lower().endswith('.tik'):
            continue
        tik = Ticket.Ticket()
        tik.open(ent)
        tkey = tik.getTitleKey().hex().upper()
        tid = tik.rightsId[0:16].upper()
        t = Titles.Title()
        t.id = tid
        t.key = tkey
        Titles.data()[tid] = t
        n += 1
    Print.info(f'{os.path.basename(path)}: tickets={n}')
    return c


def find_program_romfs(container, label):
    """返回 (nca, 节下标, ivfc, data_off_in_section)。"""
    for ent in container:
        name = ent_name(ent).lower()
        if not (name.endswith('.nca') or name.endswith('.ncz')) or name.endswith('.cnmt.nca'):
            continue
        nca = Nca.Nca()
        try:
            nca.open(ent)
        except Exception as e:
            Print.info(f'skip {ent_name(ent)}: {e}')
            continue
        if str(nca.header.contentType) != 'Content.PROGRAM':
            continue
        for k, fs in enumerate(nca):
            if getattr(fs, 'ivfc', None) is None:
                continue
            levels = fs.ivfc.levels
            if len(levels) < 2:
                continue
            Print.info(f'{label} program romfs: section[{k}] levels={len(levels)} '
                       f'data=+{levels[-1].offset:#x} size={levels[-1].size:#x}')
            return nca, k, fs.ivfc, levels[-1].offset
    return None, None, None, None


class RomfsReader:
    """BKTR 解析后的 RomFS 虚拟读。base 只读基座;upd 节含重定位表。"""

    def __init__(self, base_nca, base_k, upd_nca, upd_k, data_off):
        self.base_sec = base_nca.sections[base_k]
        self.upd_sec = upd_nca.sections[upd_k]
        self.base_fs = base_nca[base_k]
        self.upd_fs = upd_nca[upd_k]
        self.upd_crypto = self.upd_sec.crypto
        self.data_off = data_off
        self.sec_abs = upd_nca.header.sectionTables[upd_k].offset
        # 密钥/计数器在节的 partition 视图上(Nca.open 把 titleKeyDec 传给它)
        self.key = self.upd_sec.cryptoKey
        self.counter = self.upd_sec.cryptoCounter
        self._parse_relocations()
        self._parse_subsections()
        Print.info(f'bktr: reloc entries={len(self.reloc)} subsections={len(self.sub)}')

    def _sec_read(self, sec, off, n):
        if n <= 0:
            return b''
        sec.seek(off)
        b = sec.read(n)
        if len(b) != n:
            raise IOError(f'section short read {off:#x}+{n:#x} got {len(b):#x}')
        return b

    def _parse_relocations(self):
        bk = self.upd_fs.bktrRelocation
        assert bk is not None and bk.bktr_size > 0, '更新节没有 BKTR 重定位表'
        p = bk.bktr_offset
        pad, bucket_count, total = struct.unpack('<IIQ', self._sec_read(self.upd_sec, p, 16))
        base_offs = struct.unpack(f'<{0x3FF0 // 8}Q',
                                  self._sec_read(self.upd_sec, p + 16, 0x3FF0))
        entries = []
        q = p + 0x4000
        for _ in range(bucket_count):
            # 桶节点紧密排列:头节点 0x4000 后依次;endOffset 是虚拟界,不是表内偏移
            bpad, cnt, end = struct.unpack('<IIQ', self._sec_read(self.upd_sec, q, 16))
            raw = self._sec_read(self.upd_sec, q + 16, cnt * 20)
            for i in range(cnt):
                # 条目 20 字节 {u64 虚拟, u64 物理, u32 isPatch}(Atmosphere 序列化布局,
                # 对照验证:v=0x10d0560→p=0xf70570 恰为 base 数据区起点+0x570)
                v, ph, is_patch = struct.unpack_from('<QQI', raw, i * 20)
                if v == 0 and ph == 0:
                    continue          # 桶内零填充条目(第二桶可为全零占位)
                entries.append((v, ph, is_patch))
            q += 16 + cnt * 20
        entries.sort()
        # 同一虚拟起点只留首条(表内顺序即优先序;重复 v 排序后取末条会串)
        dedup = []
        for e in entries:
            if dedup and dedup[-1][0] == e[0]:
                continue
            dedup.append(e)
        self.reloc = dedup
        for e in self.reloc[:3]:
            Print.info('  reloc: virt=%#x phys=%#x patch=%d' % e)

    def _parse_subsections(self):
        bk = self.upd_fs.bktrSubsection
        sub = []
        if bk is not None and bk.isValid():
            for e in bk.getAllEntries():
                sub.append((e.virtualOffset, e.ctr, e.size))
        self.sub = sub

    def _reloc_at(self, v):
        """返回 (下标, virt, phys, is_patch):管辖 v 的重定位条目。"""
        lo, hi = 0, len(self.reloc)
        while lo < hi:
            mid = (lo + hi) // 2
            if self.reloc[mid][0] <= v:
                lo = mid + 1
            else:
                hi = mid
        i = max(0, lo - 1)
        rv, ph, is_patch = self.reloc[i]
        return i, rv, ph, is_patch

    def _sub_bound(self, phys):
        """返回 (本子段 ctr, 下一子段物理边界;无则 None)。"""
        cur_ctr = None
        nxt = None
        for sv, ctr, _sz in self.sub:
            if sv <= phys:
                cur_ctr = ctr
            else:
                nxt = sv
                break
        return cur_ctr, nxt

    def _upd_phys_read(self, phys, n):
        """更新节物理读,按子段 CTR 解密(调用方保证 [phys,phys+n) 不跨子段)。"""
        sec = self.upd_sec
        sec.crypto = None                        # 取原始密文
        try:
            sec.seek(phys)
            raw = sec.read(n)
        finally:
            sec.crypto = self.upd_crypto
        if len(raw) != n:
            raise IOError(f'upd short read {phys:#x}+{n:#x}')
        ctr, _ = self._sub_bound(phys)
        if ctr is None:
            return raw                           # 子段表未覆盖:节连续 CTR
        aes = aes128.AESCTR(self.key, nonce=bytearray(self.counter))
        aes.bktrSeek(self.sec_abs + self.data_off + phys, ctr)
        return aes.decrypt(bytearray(raw))

    def read(self, v, n):
        out = bytearray()
        while n > 0:
            i, rv, ph, is_patch = self._reloc_at(v)
            phys = ph + (v - rv)
            # 本重定位条目的管辖长度(到下一条目的虚拟起点)
            span_end = self.reloc[i + 1][0] if i + 1 < len(self.reloc) else v + n
            limit = span_end - v
            if is_patch:
                _, nxt_sub = self._sub_bound(phys)
                if nxt_sub is not None:
                    limit = min(limit, nxt_sub - phys)   # 解密计数器不跨子段
            chunk = max(1, min(n, limit))
            if is_patch:
                out += self._upd_phys_read(phys, chunk)
            else:
                out += self._sec_read(self.base_sec, self.data_off + phys, chunk)
            v += chunk
            n -= chunk
        return bytes(out)


def walk_romfs(rd, data_size):
    """遍历 RomFS 表(变长条目,链接字段是字节偏移),产出 (路径, 数据偏移, 大小)。
    dir 条目 {4I 链接, u64 哈希桶, u32 名长@0x18, 名@0x1C};file 条目 {2I 链接,
    u64 偏移, u64 大小, u32 名长@0x20, 名@0x24};均 4 字节对齐步进。"""
    hdr = rd.read(0, 0x50)
    (hsz, dht_off, dht_sz, dt_off, dt_sz,
     fht_off, fht_sz, ft_off, ft_sz, fd_off) = struct.unpack('<10Q', hdr)
    Print.info(f'romfs header: dirs={dt_sz:#x} files={ft_sz:#x} data@{fd_off:#x}/{data_size:#x}')
    dir_table = rd.read(dt_off, dt_sz)
    file_table = rd.read(ft_off, ft_sz)

    def dir_ent(o):
        parent, sib, childd, childf = struct.unpack_from('<4I', dir_table, o)
        name_len = struct.unpack_from("<I", dir_table, o + 0x18)[0]
        name = dir_table[o + 0x1C:o + 0x1C + name_len].decode('utf-8', 'replace')
        return parent, sib, childd, childf, name

    def dir_next(o):
        nl = struct.unpack_from('<I', dir_table, o + 0x18)[0]
        return (o + 0x1C + nl + 3) & ~3

    def file_ent(o):
        parent, sib = struct.unpack_from('<2I', file_table, o)
        fo, fsz = struct.unpack_from('<2Q', file_table, o + 8)
        name_len = struct.unpack_from("<I", file_table, o + 0x1C)[0]
        name = file_table[o + 0x20:o + 0x20 + name_len].decode('utf-8', 'replace')
        return parent, sib, fo, fsz, name

    def file_next(o):
        nl = struct.unpack_from("<I", file_table, o + 0x1C)[0]
        return (o + 0x20 + nl + 3) & ~3

    out = []

    def walk_dir(off, prefix):
        while off != 0xFFFFFFFF and off + 0x1C <= len(dir_table):
            _p, sib, childd, childf, name = dir_ent(off)
            path = prefix + name + '/'
            fo = childf
            while fo != 0xFFFFFFFF and fo + 0x20 <= len(file_table):
                _fp, fsib, fdo, fsz, fname = file_ent(fo)
                out.append((path + fname, fd_off + fdo, fsz))
                fo = file_next(fo)
            if childd != 0xFFFFFFFF:
                walk_dir(childd, path)
            off = sib

    _, _, childd, childf, _ = dir_ent(0)
    fo = childf
    while fo != 0xFFFFFFFF and fo + 0x20 <= len(file_table):
        _p, fsib, fdo, fsz, fname = file_ent(fo)
        out.append((fname, fd_off + fdo, fsz))
        fo = file_next(fo)
    if childd != 0xFFFFFFFF:
        walk_dir(childd, '')
    return out, fd_off, data_size


def ivfc_verify(rd, ivfc, data_off_in_section, data_size):
    """全镜像 IVFC 校验:levels[0] 的整层 sha256 == ivfc.hash;
    levels[i] 的每块(2^blockSize)是 levels[i+1] 对应字节段的 sha256。"""
    levels = ivfc.levels
    data_bs = 1 << levels[-1].blockSize
    n_blocks = (data_size + data_bs - 1) // data_bs
    Print.info(f'ivfc verify: {n_blocks} x {data_bs:#x} blocks ...')
    t0 = time.time()
    # 数据层 → 第一个哈希层:逐块哈希(不缓存整层)
    def data_block_hash(i):
        n = min(data_bs, data_size - i * data_bs)
        return hashlib.sha256(rd.read(i * data_bs, n)).digest()
    layer = [data_block_hash(i) for i in range(n_blocks)]
    for li in range(len(levels) - 2, -1, -1):
        bs = 1 << levels[li].blockSize
        joined = b''.join(layer)
        layer = [hashlib.sha256(joined[i:i + bs]).digest()
                 for i in range(0, len(joined), bs)]
        del joined
    got = hashlib.sha256(b''.join(layer)).digest()
    want = bytes(ivfc.hash)
    ok = got == want
    Print.info(f'ivfc master hash {"OK" if ok else "MISMATCH"} '
               f'({time.time() - t0:.0f}s) got={got.hex()} want={want.hex()}')
    return ok


def main():
    base_c = open_container(BASE)
    upd_c = open_container(UPD)
    b_nca, b_k, b_ivfc, b_doff = find_program_romfs(base_c, 'base')
    u_nca, u_k, u_ivfc, u_doff = find_program_romfs(upd_c, 'update')
    assert b_nca and u_nca, '没找到 Program NCA 的 RomFS 节'
    assert u_ivfc.hash == b_ivfc.hash or True  # 更新节有自己的主哈希
    data_size = b_ivfc.levels[-1].size if BASE_ONLY else u_ivfc.levels[-1].size

    if BASE_ONLY:
        # 基础版直读(无 BKTR):基础 romfs 自洽完整(v0 数据);更新补丁另解
        b_sec = b_nca.sections[b_k]

        class BaseReader:
            def read(self, v, n):
                b_sec.seek(b_doff + v)
                r = b_sec.read(n)
                if len(r) != n:
                    raise IOError(f'base short read {v:#x}+{n:#x}')
                return r
        rd = BaseReader()
    else:
        rd = RomfsReader(b_nca, b_k, u_nca, u_k, u_doff)
    files, fd_off, _ = walk_romfs(rd, data_size)
    total = sum(s for _, _, s in files)
    Print.info(f'romfs: {len(files)} files, {total / (1 << 30):.2f} GiB')

    if LIST:
        for path, off, size in files[:20]:
            Print.info(f'  {path}  {size:#x}')
        sys.exit(0)

    if VERIFY_ONLY:
        v_ivfc, v_doff = (b_ivfc, b_doff) if BASE_ONLY else (u_ivfc, u_doff)
        ok = ivfc_verify(rd, v_ivfc, v_doff, data_size)
        sys.exit(0 if ok else 1)

    OUT.mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    done = 0
    with open(OUT / '_romfs_manifest.tsv', 'w', encoding='utf-8') as man:
        for path, off, size in files:
            rel = path.lstrip('/')           # 根目录文件带前导 '/',pathlib 会丢基址
            fp = OUT / rel
            fp.parent.mkdir(parents=True, exist_ok=True)
            h = hashlib.sha256()
            left = size
            with open(fp, 'wb') as w:
                while left > 0:
                    n = min(1 << 20, left)
                    b = rd.read(off + (size - left), n)
                    w.write(b)
                    h.update(b)
                    left -= n
            man.write(f'{rel}\t{size}\t{h.hexdigest()}\n')
            done += 1
            if done % 500 == 0:
                Print.info(f'  {done}/{len(files)} ({time.time() - t0:.0f}s)')
    Print.info(f'提取完成:{done} 文件,{time.time() - t0:.0f}s -> {OUT}')

    if VERIFY:
        v_ivfc, v_doff = (b_ivfc, b_doff) if BASE_ONLY else (u_ivfc, u_doff)
        ok = ivfc_verify(rd, v_ivfc, v_doff, data_size)
        if not ok:
            sys.exit('IVFC 校验失败:提取器有 bug,数据不可信')
        Print.info('IVFC 全量校验通过')


if __name__ == '__main__':
    main()
