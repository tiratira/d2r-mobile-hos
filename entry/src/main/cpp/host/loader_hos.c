/*
 * loader_hos.c — 宿主扮演 Horizon rtld(上游 dm/host/loader.c 的 HOS 移植)。
 *
 * 原地装载:镜像编在 libdmhost.so 的节里(arm64 "ax" 原地执行 —— `bl dm_gate`
 * 的 PC 相对偏移因此保持有效;镜像起始 16K 对齐)。装载器只对数据窗口
 * [ro_off, patch_off) mprotect RW 做重定位,代码区([0,text_end) 与补丁区)
 * 始终是 .so 签名页,不产生任何运行时代码。x86_64 构建编 data-only 镜像
 * (纯数据),同一套解析/重定位代码可跑、可测,只是不执行。
 *
 * 流程照上游 load_modules:先全部登记并解析符号表,再统一重定位(跨模块
 * 符号要先都在位);重定位语义照上游 relocate():
 *   R_AARCH64_RELATIVE → base + addend
 *   JUMP_SLOT / GLOB_DAT / ABS64 → 强制接管(swforce)> 模块内定义 >
 *                                  跨模块定义 > HLE 段(swhle)> 记名桩
 */
#define _GNU_SOURCE
#include "host/dm.h"
#include "loader/dm_guest_loader.h"

#include <elf.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef R_AARCH64_ABS64
#define R_AARCH64_ABS64 257
#define R_AARCH64_GLOB_DAT 1025
#define R_AARCH64_JUMP_SLOT 1026
#define R_AARCH64_RELATIVE 1027
#endif

static size_t dm_ps(void)
{
    static size_t ps;
    if (!ps) {
        long v = sysconf(_SC_PAGESIZE);
        ps = v > 0 ? (size_t)v : 0x1000;
    }
    return ps;
}

/* ---------- 动态段 / 符号表(照上游 parse_dynamic / build_symhash) ---------- */
static uint32_t sym_hash(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s) {
        h = (h ^ (uint8_t)*s++) * 16777619u;
    }
    return h;
}

static void parse_dynamic(dm_module_t *m)
{
    uint32_t mod0;
    memcpy(&mod0, m->base + 4, 4);
    if (memcmp(m->base + mod0, "MOD0", 4) != 0) {
        die("模块没有 MOD0");
    }
    int32_t dyn_off;
    memcpy(&dyn_off, m->base + mod0 + 4, 4);
    m->dyn = m->base + mod0 + dyn_off;
    for (const Elf64_Dyn *d = m->dyn; d->d_tag; d++) {
        switch (d->d_tag) {
        case DT_SYMTAB:
            m->symtab = m->base + d->d_un.d_ptr;
            break;
        case DT_STRTAB:
            m->strtab = (const char *)(m->base + d->d_un.d_ptr);
            break;
        case DT_INIT:
            m->init = d->d_un.d_ptr;
            break;
        case DT_INIT_ARRAY:
            m->init_array = d->d_un.d_ptr;
            break;
        case DT_INIT_ARRAYSZ:
            m->init_arraysz = d->d_un.d_val;
            break;
        default:
            break;
        }
    }
    m->nsym = (uint32_t)(((const uint8_t *)m->strtab - (const uint8_t *)m->symtab) /
                         sizeof(Elf64_Sym));
    /* 名字哈希:按下标升序插入、同名不覆盖 ⇒ 与线性扫描(首个已定义同名)一致 */
    unsigned cap = 16;
    while (cap < m->nsym * 2) {
        cap <<= 1;
    }
    m->symh = calloc(cap, sizeof *m->symh);
    m->symh_mask = cap - 1;
    for (unsigned i = 1; i < m->nsym; i++) {
        const Elf64_Sym *s = &((const Elf64_Sym *)m->symtab)[i];
        if (!s->st_shndx) {
            continue;
        }
        const char *nm = m->strtab + s->st_name;
        for (unsigned j = sym_hash(nm) & m->symh_mask;; j = (j + 1) & m->symh_mask) {
            if (!m->symh[j]) {
                m->symh[j] = i;
                break;
            }
            const Elf64_Sym *old = &((const Elf64_Sym *)m->symtab)[m->symh[j]];
            if (strcmp(m->strtab + old->st_name, nm) == 0) {
                break;   /* 同名更小下标已占:保留 */
            }
        }
    }
}

static uint8_t *module_sym(dm_module_t *m, const char *name)
{
    for (unsigned j = sym_hash(name) & m->symh_mask; m->symh[j];
         j = (j + 1) & m->symh_mask) {
        const Elf64_Sym *s = &((const Elf64_Sym *)m->symtab)[m->symh[j]];
        if (strcmp(m->strtab + s->st_name, name) == 0) {
            return m->base + s->st_value;
        }
    }
    return NULL;
}

uint8_t *dm_guest_sym(const char *name)
{
    for (int i = 0; i < dm_max_modules(); i++) {
        dm_module_t *m = dm_module_at(i);
        if (m->symh) {
            uint8_t *p = module_sym(m, name);
            if (p) {
                return p;
            }
        }
    }
    return NULL;
}

/* ---------- 装载 ---------- */
int dm_load_module(const void *image, const void *meta, const char *name)
{
    DmGuestModule g;
    if (dmGuestLoad(image, meta, name, &g) != 0 || dmGuestVerify(&g) != 0) {
        return -1;
    }
    dm_module_register(name, g.image, g.hdr.image_size);
    dm_module_t *m = dm_module_find(name);
    if (m == NULL) {
        return -1;
    }
    m->patch_off = g.hdr.patch_off;
    m->text_end = g.hdr.text_end;
    m->ro_off = g.hdr.ro_off;
    m->data_off = g.hdr.data_off;

    /* 数据窗口 RW(页对齐下调;窗口里含 ro 尾与键页,补丁区界是 16K 对齐的)。
     * 代码区不写:重定位目标全在 ro/data 段,补丁区本来就不动。 */
    size_t ps = dm_ps();
    uint64_t lo = m->ro_off & ~(uint64_t)(ps - 1);
    if (mprotect(m->base + lo, m->patch_off - lo, PROT_READ | PROT_WRITE) != 0) {
        LOG("mprotect data window %{public}s [+%{public}llx,%{public}llx) failed errno=%{public}d",
            name, (unsigned long long)lo, (unsigned long long)m->patch_off, errno);
        return -1;
    }
    parse_dynamic(m);

    /* 门表:返回点/原指令都在镜像内,登记绝对地址(原地,即节内地址) */
    for (uint32_t i = 0; i < g.hdr.gate_count; i++) {
        DmGuestGate gt;
        if (dmGuestGateAt(&g, i, &gt) == 0) {
            gate_register_abs((uint64_t)(m->base + gt.ret_off), (int)gt.kind, gt.arg,
                              (uint64_t)(m->base + gt.orig_off));
        }
    }
    LOG("loaded %{public}s: base=%{public}p size=%{public}llx sym=%{public}u gates=%{public}u "
        "patch=%{public}llx text_end=%{public}llx",
        name, m->base, (unsigned long long)m->size, m->nsym, g.hdr.gate_count,
        (unsigned long long)m->patch_off, (unsigned long long)m->text_end);
    return 0;
}

/* ---------- 重定位(照上游 relocate) ---------- */
static uint64_t resolve_sym(dm_module_t *m, const char *name, int weak)
{
    for (int i = 0; i < dm_max_modules(); i++) {
        dm_module_t *o = dm_module_at(i);
        if (o->symh) {
            uint8_t *p = module_sym(o, name);
            if (p) {
                return (uint64_t)(uintptr_t)p;
            }
        }
    }
    return resolve_import(name, weak);
}

void dm_relocate_all(void)
{
    for (int mi = 0; mi < dm_max_modules(); mi++) {
        dm_module_t *m = dm_module_at(mi);
        if (!m->dyn) {
            continue;
        }
        uint64_t rela = 0, relasz = 0, jmprel = 0, pltsz = 0;
        for (const Elf64_Dyn *d = m->dyn; d->d_tag; d++) {
            switch (d->d_tag) {
            case DT_RELA:
                rela = d->d_un.d_ptr;
                break;
            case DT_RELASZ:
                relasz = d->d_un.d_val;
                break;
            case DT_JMPREL:
                jmprel = d->d_un.d_ptr;
                break;
            case DT_PLTRELSZ:
                pltsz = d->d_un.d_val;
                break;
            default:
                break;
            }
        }
        const Elf64_Rela *tabs[2] = {
            (const Elf64_Rela *)(m->base + rela),
            (const Elf64_Rela *)(m->base + jmprel),
        };
        uint64_t sizes[2] = {relasz, pltsz};
        for (int t = 0; t < 2; t++) {
            for (uint64_t i = 0; i < sizes[t] / sizeof(Elf64_Rela); i++) {
                const Elf64_Rela *r = &tabs[t][i];
                uint64_t *where = (uint64_t *)(m->base + r->r_offset);
                unsigned type = ELF64_R_TYPE(r->r_info);
                unsigned si = ELF64_R_SYM(r->r_info);
                if (type == R_AARCH64_RELATIVE) {
                    *where = (uint64_t)(uintptr_t)m->base + r->r_addend;
                    m->reloc_rel++;
                    continue;
                }
                const Elf64_Sym *s = &((const Elf64_Sym *)m->symtab)[si];
                const char *nm = m->strtab + s->st_name;
                uint64_t v = (uint64_t)(uintptr_t)dm_swforce_lookup(nm);   /* 强制接管 */
                if (!v) {
                    v = s->st_shndx ? (uint64_t)(uintptr_t)(m->base + s->st_value)
                                    : resolve_sym(m, nm, ELF64_ST_BIND(s->st_info) == STB_WEAK);
                }
                if (type == R_AARCH64_JUMP_SLOT) {
                    *where = v;
                    m->reloc_jmp++;
                } else {   /* GLOB_DAT / ABS64 */
                    *where = v + r->r_addend;
                    m->reloc_abs++;
                }
            }
        }
        LOG("%{public}s: reloc relative=%{public}d abs/glob=%{public}d jump=%{public}d",
            m->name, m->reloc_rel, m->reloc_abs, m->reloc_jmp);
    }
}

/* ---------- 自检报告 ---------- */
int dm_loader_report(char *buf, int size)
{
    int off = 0;
    for (int i = 0; i < dm_max_modules(); i++) {
        dm_module_t *m = dm_module_at(i);
        off += snprintf(buf + off, size - off, "%s:%llusym/r%d+%d+j%d ",
                        m->name, (unsigned long long)m->nsym,
                        m->reloc_rel, m->reloc_abs, m->reloc_jmp);
    }
    /* SDK 启动序关键符号(上游 main.c guest_main 逐条照抄的那串) */
    static const char *const probe_syms[] = {
        "_init_libc0", "nnosInitialize", "_init_libc1",
        "nninitInitializeSdkModule", "nninitStartup", "_init_libc2",
        "nnMain",
    };
    int found = 0;
    const int n = (int)(sizeof probe_syms / sizeof probe_syms[0]);
    for (int i = 0; i < n; i++) {
        uint8_t *p = dm_guest_sym(probe_syms[i]);
        if (p) {
            found++;
        }
    }
    off += snprintf(buf + off, size - off, "| boot syms %d/%d", found, n);
    return 0;
}

/* ---------- 各模块 DT_INIT(扮演 rtld 的 InitModules) ----------
 * 时机照 nn::init::Start:nninitStartup 与 _init_libc2 之后、nnMain 之前。
 * init_array 同理:DT_INIT 就是 rtld 调 init_array 的入口。 */
void run_module_inits(void)
{
    for (int i = 0; i < dm_max_modules(); i++) {
        dm_module_t *m = dm_module_at(i);
        if (m->init == 0) {
            continue;
        }
        DMLOG_I("%{public}s: DT_INIT +%{public}llx(init_array %{public}llu 项)",
                m->name, (unsigned long long)m->init,
                (unsigned long long)(m->init_arraysz / 8));
        ((void (*)(void))(m->base + m->init))();
    }
}

/* ---------- 导入解析覆盖统计 ----------
 * 装载/重定位完成后扫每个模块的 dynsym,把未定义符号按解析去向分类。
 * 真机上「落记名桩」的名字就是下一批 HLE 的排期表;x86_64 data-only
 * 构建同样能跑,把「还差什么」量化成一条可回归的报告。 */
#define IMP_MAX_NAME 72
#define IMP_MAX_UNIQ 4096
#define IMP_STUB_LIST 12
static int s_imp_done;
static int s_imp_total, s_imp_mod, s_imp_force, s_imp_hle, s_imp_weak, s_imp_stub;
static char s_imp_stub_names[IMP_STUB_LIST][IMP_MAX_NAME];
static int s_imp_stub_shown;

static int imp_seen(const char seen[][IMP_MAX_NAME], int n, const char *name)
{
    for (int i = 0; i < n; i++) {
        if (strcmp(seen[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void import_scan(void)
{
    if (s_imp_done) {
        return;
    }
    s_imp_done = 1;
    static char seen[IMP_MAX_UNIQ][IMP_MAX_NAME];
    int nseen = 0;
    for (int mi = 0; mi < dm_max_modules(); mi++) {
        dm_module_t *m = dm_module_at(mi);
        if (!m->symh || !m->symtab) {
            continue;
        }
        for (uint32_t i = 1; i < m->nsym; i++) {
            const Elf64_Sym *s = &((const Elf64_Sym *)m->symtab)[i];
            if (s->st_shndx != 0 || s->st_name == 0) {
                continue;   /* 只看未定义导入 */
            }
            const char *nm = m->strtab + s->st_name;
            if (!nm[0] || strcmp(nm, "__dm_placeholder__") == 0) {
                continue;
            }
            if (nseen < IMP_MAX_UNIQ && imp_seen((const char (*)[IMP_MAX_NAME])seen, nseen, nm)) {
                continue;
            }
            if (nseen < IMP_MAX_UNIQ) {
                snprintf(seen[nseen++], IMP_MAX_NAME, "%s", nm);
            }
            s_imp_total++;
            int weak = ELF64_ST_BIND(s->st_info) == STB_WEAK;
            if (weak && !hle_lookup(nm) && !dm_swforce_lookup(nm)) {
                s_imp_weak++;   /* 弱导入未解析保持 0(上游同语义) */
                continue;
            }
            if (dm_swforce_lookup(nm)) {
                s_imp_force++;
            } else if (dm_guest_sym(nm)) {
                s_imp_mod++;
            } else if (hle_lookup(nm)) {
                s_imp_hle++;
            } else {
                s_imp_stub++;
                if (s_imp_stub_shown < IMP_STUB_LIST) {
                    snprintf(s_imp_stub_names[s_imp_stub_shown++], IMP_MAX_NAME, "%s", nm);
                }
            }
        }
    }
    uint32_t nhle = 0, nforce = 0;
    hle_registered_counts(&nhle, &nforce);
    DMLOG_I("imports: total=%{public}d module=%{public}d force=%{public}d hle=%{public}d "
            "weak=%{public}d stub=%{public}d | registries hle=%{public}u force=%{public}u",
            s_imp_total, s_imp_mod, s_imp_force, s_imp_hle, s_imp_weak, s_imp_stub,
            (unsigned)nhle, (unsigned)nforce);
    for (int i = 0; i < s_imp_stub_shown; i++) {
        DMLOG_W("UNIMPL import: %{public}s", s_imp_stub_names[i]);
    }
}

int dm_import_report(char *buf, int size)
{
    import_scan();
    int off = snprintf(buf, size,
                       "imp:%d 模块%d 接管%d HLE%d 弱%d 桩%d",
                       s_imp_total, s_imp_mod, s_imp_force, s_imp_hle,
                       s_imp_weak, s_imp_stub);
    if (s_imp_stub_shown > 0 && off < size) {
        off += snprintf(buf + off, size - off, " [");
        for (int i = 0; i < s_imp_stub_shown && off < size; i++) {
            off += snprintf(buf + off, size - off, "%s%s", i ? "," : "",
                            s_imp_stub_names[i]);
        }
        snprintf(buf + off, size - off, "]");
    }
    return 0;
}

int dm_import_stub_count(void)
{
    import_scan();
    return s_imp_stub;
}

int dm_import_total_count(void)
{
    import_scan();
    return s_imp_total;
}
