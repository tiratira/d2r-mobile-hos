/* NVN semantics adapted from d2r-mobile hle_nvn.c (MIT). */
#include "dm_nvn.h"
#include "dm_replay.h"
#include "host/dm.h"
#include "nvn_obj.h"
#include "nvn_protos.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <unistd.h>
#define B(x) ((x)&255)
#define OLOG(...) ((void)0)
static void runtime_die(const char *fmt, ...) {
    char b[512];
    va_list a;
    va_start(a, fmt);
    vsnprintf(b, sizeof b, fmt, a);
    va_end(a);
    DMLOG_E("NVN: %{public}s", b);
    abort();
}
#define die runtime_die
static void *nvn_lookup(const char *name) { return (void *)(uintptr_t)dmNvnBootstrapLoader(name); }
typedef struct {
    const char *name;
    nvn_func_t fn;
} runtime_ent;
#define NVN(name, ret, ...)                                                                                  \
    static ret h_##name(__VA_ARGS__);                                                                        \
    static const NVNT_##name abi_##name __attribute__((unused)) = h_##name;                                  \
    __attribute__((used, section("nvn_runtime"), aligned(8))) static const runtime_ent ent_##name = {        \
        #name, (nvn_func_t)h_##name};                                                                        \
    static ret h_##name(__VA_ARGS__)
#define NVN_NOPROTO(name, ret, ...)                                                                          \
    static ret h_##name(__VA_ARGS__);                                                                        \
    __attribute__((used, section("nvn_runtime"), aligned(8))) static const runtime_ent ent_##name = {        \
        #name, (nvn_func_t)h_##name};                                                                        \
    static ret h_##name(__VA_ARGS__)
static uint64_t nvn_new_serial(void) {
    static _Atomic uint64_t n;
    return ++n;
}
static void x_drain(void) {}
static uint64_t x_fence(void) { return dmReplaySerial(); }
uint64_t dm_nvn_frame_count(void) { return dmReplayFrames(); }
NVN(nvnDeviceBuilderSetDefaults, void, NVNdeviceBuilder *b) { memset(b, 0, sizeof(dev_builder_t)); }
NVN(nvnDeviceBuilderSetFlags, void, NVNdeviceBuilder *b, int f) { ((dev_builder_t *)b)->flags = f; }
NVN(nvnDeviceInitialize, NVNboolean, NVNdevice *d, const NVNdeviceBuilder *b) {
    dev_t_ *o = (dev_t_ *)d;
    memset(o, 0, sizeof *o);
    o->magic = MAGIC('D', 'E', 'V', 0);
    o->flags = ((const dev_builder_t *)b)->flags;
    OLOG("DeviceInitialize %p flags=%#x", (void *)d, o->flags);
    return 1;
}
NVN(nvnDeviceFinalize, void, NVNdevice *d) { ((dev_t_ *)d)->magic = 0; }
NVN(nvnDeviceGetProcAddress, PFNNVNGENERICFUNCPTRPROC, const NVNdevice *d, const char *name) {
    (void)d;
    return (PFNNVNGENERICFUNCPTRPROC)nvn_lookup(name);
}

// 设备属性：Tegra X1 实测值（nnheaders nvn_DeviceConstantsNX.h，在 SMO 上逐项调 nvnDeviceGetInteger 得到）
// Device properties: values measured on Tegra X1 (nnheaders nvn_DeviceConstantsNX.h, obtained by calling
// nvnDeviceGetInteger item by item on SMO)
static const int g_devinfo[] = {
    [0x00] = 53,        [0x01] = 311, // API 版本：游戏读了不比较（main 0x173cbd8），此值未核 | API version:
                                      // the game reads it without comparing (main 0x173cbd8); this value is
                                      // unverified
    [0x02] = 14,        [0x03] = 65536, [0x04] = 256,   [0x05] = 8,     [0x06] = 16,      [0x07] = 4,
    [0x08] = 16,        [0x09] = 32,    [0x0A] = 16,    [0x0B] = 4,     [0x0C] = 4,       [0x0D] = 4,
    [0x0E] = 16,        [0x0F] = 32,    [0x10] = 32,    [0x11] = 256,   [0x12] = 256,     [0x13] = 4,
    [0x14] = 8,         [0x15] = 512,   [0x16] = 256,   [0x17] = 2,     [0x18] = 256,     [0x19] = 4096,
    [0x1A] = 131072,    [0x1B] = 16,    [0x1C] = 1536,  [0x1D] = 1024,  [0x1E] = 64,      [0x1F] = 1536,
    [0x20] = 65535,     [0x21] = 65535, [0x22] = 65535, [0x23] = 8,     [0x24] = 1048576, [0x25] = 4096,
    [0x26] = 16,        [0x27] = 4096,  [0x28] = 1,     [0x29] = 1,     [0x2A] = 1,       [0x2B] = 262144,
    [0x2C] = 15,        [0x2D] = 2048,  [0x2E] = 1,     [0x2F] = 1,     [0x30] = 14,      [0x31] = 5,
    [0x32] = 1,         [0x33] = 8,     [0x34] = 8,     [0x35] = 4,     [0x36] = 32,      [0x37] = 128,
    [0x38] = 32,        [0x39] = 128,   [0x3A] = 65536, [0x3B] = 1,     [0x3C] = 4,       [0x3D] = 16384,
    [0x3E] = 134217728, [0x3F] = 2048,  [0x40] = 16384, [0x41] = 16384, [0x42] = 1,       [0x43] = 1,
    [0x44] = 1,         [0x45] = 1,     [0x46] = 5,     [0x47] = 1,     [0x48] = 1,       [0x49] = 1,
    [0x4A] = 1,         [0x4B] = 1,     [0x4C] = 1,     [0x4D] = 16,    [0x4E] = 1024,    [0x4F] = 32,
    [0x50] = 4096,      [0x51] = 65536, [0x52] = 65536, [0x53] = 256,   [0x54] = 16384,   [0x55] = 262144,
    [0x56] = 4096,      [0x57] = 1,     [0x58] = 4,     [0x59] = 2,     [0x5A] = 0,       [0x5B] = 4096,
    [0x5C] = 16384,     [0x5D] = 4096,  [0x5E] = 128,   [0x5F] = 4096,
};
NVN(nvnDeviceGetInteger, void, const NVNdevice *d, NVNdeviceInfo what, int *out) {
    (void)d;
    unsigned w = (unsigned)what;
    // 0x5F 分离采样器绑定数、0x60/0x61 在 SMO 实测表里没有：0x5F 按采样器池上限给 4096 未核，0x60/0x61 报红给
    // 0 0x5F separate sampler binding count and 0x60/0x61 are not in the table measured on SMO: 0x5F is given
    // 4096 per the sampler pool limit, unverified; 0x60/0x61 report a failure and return 0
    if (w >= sizeof g_devinfo / sizeof *g_devinfo) {
        LOG("【红】nvnDeviceGetInteger 未知项 %#x，给 0", w);
        *out = 0;
        return;
    }
    *out = g_devinfo[w];
    OLOG("DeviceGetInteger %#x -> %d", w, *out);
}
NVN(nvnDeviceGetImageHandle, NVNimageHandle, const NVNdevice *d, int id) {
    (void)d;
    return (NVNimageHandle)id;
}
// 分离贴图 / 采样器句柄：值 = 描述符号（采样器号放 20 位以上，与组合句柄的位布局一致）
// Separate texture / sampler handles: value = descriptor index (the sampler index goes in bit 20 and above,
// consistent with the bit layout of combined handles)
NVN(nvnDeviceGetSeparateTextureHandle, NVNseparateTextureHandle, const NVNdevice *d, int id) {
    (void)d;
    return (NVNseparateTextureHandle){(uint64_t)id};
}
NVN(nvnDeviceGetSeparateSamplerHandle, NVNseparateSamplerHandle, const NVNdevice *d, int id) {
    (void)d;
    return (NVNseparateSamplerHandle){(uint64_t)id << 20};
}
NVN(nvnDeviceInstallDebugCallback, void, NVNdevice *d, const PFNNVNDEBUGCALLBACKPROC cb, void *data,
    uint32_t enable) {
    dev_t_ *o = (dev_t_ *)d;
    o->dbg_cb = B(enable) ? (void *)cb : NULL;
    o->dbg_data = data;
}
NVN(nvnDeviceSetWindowOriginMode, void, NVNdevice *d, NVNwindowOriginMode m) {
    ((dev_t_ *)d)->origin = m;
    LOG("[nvn] 窗口原点 %s", m ? "左上（同 D3D / Metal）" : "左下（GL）");
}
NVN(nvnDeviceSetDepthMode, void, NVNdevice *d, NVNdepthMode m) {
    ((dev_t_ *)d)->depth_mode = m;
    LOG("[nvn] 深度模式 %s", m ? "近=0（同 D3D / Metal）" : "近=-w（GL）");
}
NVN(nvnDeviceFinalizeCommandHandle, void, NVNdevice *d, NVNcommandHandle h) {
    (void)d;
    (void)h;
}

// ---------- 内存池 ----------
// ---------- Memory pools ----------
NVN(nvnMemoryPoolBuilderSetDefaults, void, NVNmemoryPoolBuilder *b) { memset(b, 0, sizeof(pool_builder_t)); }
NVN(nvnMemoryPoolBuilderSetDevice, void, NVNmemoryPoolBuilder *b, NVNdevice *d) {
    ((pool_builder_t *)b)->dev = d;
}
NVN(nvnMemoryPoolBuilderSetStorage, void, NVNmemoryPoolBuilder *b, void *m, size_t n) {
    ((pool_builder_t *)b)->mem = m;
    ((pool_builder_t *)b)->size = n;
}
NVN(nvnMemoryPoolBuilderSetFlags, void, NVNmemoryPoolBuilder *b, int f) { ((pool_builder_t *)b)->flags = f; }
NVN(nvnMemoryPoolInitialize, NVNboolean, NVNmemoryPool *p, const NVNmemoryPoolBuilder *b) {
    const pool_builder_t *s = (const pool_builder_t *)b;
    pool_t *o = (pool_t *)p;
    o->magic = MAGIC('P', 'O', 'O', 'L');
    o->flags = s->flags;
    o->mem = s->mem;
    o->size = s->size;
    if (o->mem) {
        mtl_pool_add(o->mem, o->size);
    }
    if (!o->mem && !(o->flags & 0x200))
        LOG("【红】MemoryPoolInitialize 非虚拟池没有存储 flags=%#x size=%#llx", o->flags, o->size);
    OLOG("MemoryPool %p mem=%p size=%#llx flags=%#x", (void *)p, (void *)o->mem, o->size, o->flags);
    return 1;
}
NVN(nvnMemoryPoolFinalize, void, NVNmemoryPool *p) {
    if (((pool_t *)p)->mem) {
        mtl_pool_del(((pool_t *)p)->mem);
    }
    ((pool_t *)p)->magic = 0;
}
NVN(nvnMemoryPoolFlushMappedRange, void, const NVNmemoryPool *p, ptrdiff_t off, size_t n) {
    (void)p;
    (void)off;
    (void)n;
}
NVN(nvnMemoryPoolGetBufferAddress, NVNbufferAddress, const NVNmemoryPool *p) {
    return (NVNbufferAddress)((const pool_t *)p)->mem;
}

// ---------- 描述符池 ----------
// ---------- Descriptor pools ----------
NVN(nvnTexturePoolInitialize, NVNboolean, NVNtexturePool *tp, const NVNmemoryPool *p, ptrdiff_t off, int n) {
    *(dpool_t *)tp = (dpool_t){(const pool_t *)p, off, n};
    OLOG("TexturePool n=%d", n);
    mtl_set_tex_pool_n(n);
    return 1;
}
NVN(nvnTexturePoolFinalize, void, NVNtexturePool *tp) { (void)tp; }
NVN(nvnTexturePoolRegisterTexture, void, const NVNtexturePool *tp, int id, const NVNtexture *t,
    const NVNtextureView *v) {
    (void)tp;
    mtl_reg_tex(id, (const tex_t *)t, (const view_t *)v);
}
NVN(nvnSamplerPoolInitialize, NVNboolean, NVNsamplerPool *sp, const NVNmemoryPool *p, ptrdiff_t off, int n) {
    *(dpool_t *)sp = (dpool_t){(const pool_t *)p, off, n};
    OLOG("SamplerPool n=%d", n);
    return 1;
}
NVN(nvnSamplerPoolFinalize, void, NVNsamplerPool *sp) { (void)sp; }
NVN(nvnSamplerPoolRegisterSampler, void, const NVNsamplerPool *sp, int id, const NVNsampler *s) {
    (void)sp;
    mtl_reg_samp(id, &((const samp_t *)s)->p);
}

// ---------- 贴图 ----------
// ---------- Textures ----------
#define TB(b) ((tex_builder_t *)(b))
NVN(nvnTextureBuilderSetDefaults, void, NVNtextureBuilder *b) {
    NVNdevice *dev = TB(b)->dev;
    memset(b, 0, sizeof(tex_builder_t));
    TB(b)->dev = dev;
    TB(b)->d.target = NVN_TEXTURE_TARGET_2D;
    TB(b)->d.width = TB(b)->d.height = TB(b)->d.depth = TB(b)->d.levels = 1;
}
NVN(nvnTextureBuilderSetDevice, void, NVNtextureBuilder *b, NVNdevice *d) { TB(b)->dev = d; }
NVN(nvnTextureBuilderSetFlags, void, NVNtextureBuilder *b, int f) { TB(b)->d.flags = f; }
NVN(nvnTextureBuilderSetTarget, void, NVNtextureBuilder *b, NVNtextureTarget t) { TB(b)->d.target = t; }
NVN(nvnTextureBuilderSetWidth, void, NVNtextureBuilder *b, int v) { TB(b)->d.width = v; }
NVN(nvnTextureBuilderSetHeight, void, NVNtextureBuilder *b, int v) { TB(b)->d.height = v; }
NVN(nvnTextureBuilderSetDepth, void, NVNtextureBuilder *b, int v) { TB(b)->d.depth = v; }
NVN(nvnTextureBuilderSetSize1D, void, NVNtextureBuilder *b, int w) {
    TB(b)->d.width = w;
    TB(b)->d.height = TB(b)->d.depth = 1;
}
NVN(nvnTextureBuilderSetLevels, void, NVNtextureBuilder *b, int v) { TB(b)->d.levels = v; }
NVN(nvnTextureBuilderSetFormat, void, NVNtextureBuilder *b, NVNformat f) { TB(b)->d.format = f; }
NVN(nvnTextureBuilderSetSamples, void, NVNtextureBuilder *b, int s) { TB(b)->d.samples = s; }
NVN(nvnTextureBuilderSetStride, void, NVNtextureBuilder *b, ptrdiff_t s) { TB(b)->d.stride = (uint32_t)s; }
NVN(nvnTextureBuilderSetStorage, void, NVNtextureBuilder *b, NVNmemoryPool *p, ptrdiff_t off) {
    TB(b)->pool = (pool_t *)p;
    TB(b)->off = off;
}
NVN(nvnTextureBuilderSetPackagedTextureLayout, void, NVNtextureBuilder *b,
    const NVNpackagedTextureLayout *l) {
    TB(b)->has_layout = l != NULL;
    if (l)
        memcpy(TB(b)->layout, l->layout, 8);
    TB(b)->d.pkg_block_h = l ? l->layout[0] + 1 : 0;
}
NVN(nvnTextureBuilderGetFlags, NVNtextureFlags, const NVNtextureBuilder *b) {
    return (NVNtextureFlags)((const tex_builder_t *)b)->d.flags;
}
static uint64_t g_layout_logged;
static void log_layout(const tex_builder_t *b, const nvn_texlayout *L) {
    if (b->has_layout && g_layout_logged++ < 16)
        LOG("[nvn] 预排布贴图 %ux%ux%u 级%u 格式%#x 目标%d 布局字节 %02x%02x%02x%02x%02x%02x%02x%02x → 块高 "
            "log2=%u（取布局第 0 字节）尺寸 %#llx",
            b->d.width, b->d.height, b->d.depth, b->d.levels, b->d.format, b->d.target, b->layout[0],
            b->layout[1], b->layout[2], b->layout[3], b->layout[4], b->layout[5], b->layout[6], b->layout[7],
            L->tileH, L->size);
}
NVN(nvnTextureBuilderGetStorageSize, size_t, const NVNtextureBuilder *b) {
    nvn_texlayout L;
    nvn_tex_layout(&((const tex_builder_t *)b)->d, &L);
    if (!L.bpb)
        LOG("【红】TextureBuilderGetStorageSize 未知格式 %#x", ((const tex_builder_t *)b)->d.format);
    log_layout((const tex_builder_t *)b, &L);
    return L.size;
}
NVN(nvnTextureBuilderGetStorageAlignment, size_t, const NVNtextureBuilder *b) {
    nvn_texlayout L;
    nvn_tex_layout(&((const tex_builder_t *)b)->d, &L);
    return L.align;
}
#define TX(t) ((tex_t *)(t))
#define CTX(t) ((const tex_t *)(t))
// ---------- 对象副本：录制时按序号留一份贴图 / 程序对象，命令载荷存副本地址 ----------
// 同一序号的对象内容不变（Initialize / SetShaders 才换序号），游戏提交后可以改写或重建自己的对象（真机上 GPU
// 只看录进命令内存的值），
// 回放层读副本就不受影响。游戏对象里记着副本地址，按值拷来的对象序号相同、直接共用。副本在对象 Finalize /
// 换序号后再过 SNAP_KEEP 次提交才回收； 槽位内存只增不还给系统，回收时清零（序号 0
// 不与任何有效序号相等），过期的副本地址只会读到不相等的序号、重新取副本。
// ---------- Object snapshots: when recording, keep one copy of each texture / program object per serial;
// command payloads store the snapshot address ---------- An object with the same serial has the same content
// (only Initialize / SetShaders change the serial); after submitting, the game may rewrite or rebuild its own
// objects (on the real device the GPU only sees the values recorded into command memory), and the replay
// layer reading snapshots is unaffected. The game object records the snapshot address, and objects copied by
// value have the same serial and share it directly. A snapshot is reclaimed only SNAP_KEEP submits after the
// object is Finalized / changes serial; slot memory only grows and is never returned to the system; reclaimed
// slots are zeroed (serial 0 never equals a valid serial), so a stale snapshot address only reads a
// non-matching serial and takes a new snapshot.
typedef union snap_slot {
    tex_t t;
    prog_t p;
    union snap_slot *next;
} snap_slot;
#define SNAP_KEEP 8
static pthread_mutex_t g_snap_mu = PTHREAD_MUTEX_INITIALIZER;
static snap_slot *g_snap_free;
static struct {
    snap_slot *s;
    uint64_t at;
} *g_snap_ret;
static size_t g_snap_nret, g_snap_capret, g_snap_live, g_snap_slots;
static uint64_t g_snap_sub; // 已执行（翻译完）的提交数：回收时刻 | Number of submits executed (translated):
                            // the reclaim clock
static _Atomic uint64_t
    g_x_subs; // 已入队的提交数（翻译队列，见 nvnQueueSubmitCommands
              // 前）：退役盖戳用它，回收按已执行数，翻译线程落后时副本不会提前回收 | Number of submits
              // enqueued (translation queue, see before nvnQueueSubmitCommands): used for the retire stamp,
              // while reclaim goes by the number executed, so snapshots are not reclaimed early when the
              // translation thread falls behind
static snap_slot *snap_alloc(void) { // 持 g_snap_mu | Holds g_snap_mu
    if (!g_snap_free) {
        snap_slot *c = calloc(256, sizeof *c);
        if (!c) {
            LOG("【红】对象副本分配失败");
            abort();
        }
        for (int i = 255; i >= 0; i--) {
            c[i].next = g_snap_free;
            g_snap_free = &c[i];
        }
        g_snap_slots +=
            256; // 读数：槽位只在在用 + 待回收占满时才扩，扩槽停住即有上界 | Reading: slots grow only when
                 // in-use + pending-reclaim fill them all; slot growth stops, so there is an upper bound
        LOG("[nvn] 对象副本槽位扩到 %zu 个（%zu KB）：在用 %zu，其中待回收 %zu，第 %llu 次提交", g_snap_slots,
            g_snap_slots * sizeof *c >> 10, g_snap_live, g_snap_nret, (unsigned long long)g_snap_sub);
    }
    snap_slot *n = g_snap_free;
    g_snap_free = n->next;
    memset(n, 0, sizeof *n);
    g_snap_live++;
    return n;
}
static void snap_retire(const void *s) { // 持 g_snap_mu | Holds g_snap_mu
    if (g_snap_nret == g_snap_capret) {
        g_snap_capret = g_snap_capret ? g_snap_capret * 2 : 256;
        g_snap_ret = realloc(g_snap_ret, g_snap_capret * sizeof *g_snap_ret);
    }
    g_snap_ret[g_snap_nret].s = (snap_slot *)s;
    g_snap_ret[g_snap_nret++].at = atomic_load(&g_x_subs);
}
static void snap_tick(uint64_t done) { // 翻译线程每执行完一次提交：回收退役时已入队的提交都执行完、且又过了
                                       // SNAP_KEEP 次提交的副本 | Each time the translation thread finishes a
                                       // submit: reclaim snapshots whose submits enqueued at retirement have
                                       // all executed and that have passed SNAP_KEEP more submits
    pthread_mutex_lock(&g_snap_mu);
    g_snap_sub = done;
    size_t k = 0;
    for (size_t i = 0; i < g_snap_nret; i++) {
        if (g_snap_ret[i].at + SNAP_KEEP <= g_snap_sub) {
            snap_slot *n = g_snap_ret[i].s;
            memset(n, 0, sizeof *n);
            n->next = g_snap_free;
            g_snap_free = n;
            g_snap_live--;
        } else
            g_snap_ret[k++] = g_snap_ret[i];
    }
    g_snap_nret = k;
    pthread_mutex_unlock(&g_snap_mu);
}
static const tex_t *snap_tex(const NVNtexture *t) {
    tex_t *o = TX(t);
    if (!o || !o->serial)
        return o; // 没初始化过的对象没有可留的内容，照旧存原址 | An object that was never initialized has no
                  // content to keep; store the original address as before
    const tex_t *s = __atomic_load_n((const tex_t **)&o->snap, __ATOMIC_ACQUIRE);
    if (s && s->serial == o->serial)
        return s;
    pthread_mutex_lock(&g_snap_mu);
    s = o->snap;
    if (!s || s->serial != o->serial) {
        snap_slot *n = snap_alloc();
        n->t = *o;
        n->t.snap = NULL;
        s = &n->t;
        __atomic_store_n((const tex_t **)&o->snap, s, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&g_snap_mu);
    return s;
}
static const prog_t *snap_prog(const NVNprogram *p) {
    prog_t *o = (prog_t *)p;
    if (!o || !o->serial)
        return o;
    const prog_t *s = __atomic_load_n((const prog_t **)&o->snap, __ATOMIC_ACQUIRE);
    if (s && s->serial == o->serial)
        return s;
    pthread_mutex_lock(&g_snap_mu);
    s = o->snap;
    if (!s || s->serial != o->serial) {
        snap_slot *n = snap_alloc();
        n->p = *o;
        n->p.snap = NULL;
        s = &n->p;
        __atomic_store_n((const prog_t **)&o->snap, s, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&g_snap_mu);
    return s;
}
static void x_drain(void);
static uint64_t x_fence(void);
// 对象 Finalize / 换序号前调：本序号的副本退役（按值拷来的同序号对象之后再录会重新取副本）
// Called before an object is Finalized / changes serial: retire this serial's snapshot (an object with the
// same serial copied by value takes a new snapshot when recorded later)
static void snap_drop(const void **pp, uint64_t ser, uint64_t snap_ser_off) {
    pthread_mutex_lock(&g_snap_mu);
    const uint8_t *s = *pp;
    uint64_t ss;
    if (s)
        memcpy(&ss, s + snap_ser_off, 8);
    if (s && ser && ss == ser)
        snap_retire(s);
    *pp = NULL;
    pthread_mutex_unlock(&g_snap_mu);
}
NVN(nvnTextureInitialize, NVNboolean, NVNtexture *t, const NVNtextureBuilder *b) {
    const tex_builder_t *s = (const tex_builder_t *)b;
    tex_t *o = TX(t);
    memset(o, 0, sizeof *o);
    o->magic = MAGIC('T', 'E', 'X', 0);
    o->d = s->d;
    o->pool = s->pool;
    o->off = s->off;
    o->has_layout = s->has_layout;
    memcpy(o->layout, s->layout, 8);
    nvn_tex_layout(&o->d, &o->L);
    o->mem = s->pool && s->pool->mem ? s->pool->mem + s->off : NULL;
    o->serial = nvn_new_serial();
    OLOG("Texture %p %ux%ux%u lv%u fmt=%#x tgt=%d flags=%#x size=%#llx", (void *)t, o->d.width, o->d.height,
         o->d.depth, o->d.levels, o->d.format, o->d.target, o->d.flags, o->L.size);
    return 1;
}
NVN(nvnTextureFinalize, void, NVNtexture *t) {
    snap_drop(&TX(t)->snap, TX(t)->serial, offsetof(tex_t, serial));
    mtl_tex_release(TX(t));
    TX(t)->magic = 0;
}
NVN(nvnTextureGetFlags, NVNtextureFlags, const NVNtexture *t) { return (NVNtextureFlags)CTX(t)->d.flags; }
NVN(nvnTextureGetWidth, int, const NVNtexture *t) { return (int)CTX(t)->d.width; }
NVN(nvnTextureGetHeight, int, const NVNtexture *t) { return (int)CTX(t)->d.height; }
NVN(nvnTextureGetDepth, int, const NVNtexture *t) { return (int)CTX(t)->d.depth; }
NVN(nvnTextureGetStorageSize, int, const NVNtexture *t) { return (int)CTX(t)->L.size; }
NVN(nvnTextureCompare, NVNboolean, const NVNtexture *a, const NVNtexture *b) {
    const tex_t *x = CTX(a), *y = CTX(b);
    return !memcmp(&x->d, &y->d, sizeof x->d) && x->mem == y->mem;
}
// 头文件无原型；main 0x174899c 一类调用点把返回值与 1 比，不等则置某状态位 0x100。含义未核，先给 1
// No prototype in the header; call sites such as main 0x174899c compare the return value with 1 and set some
// status bit 0x100 if not equal. Meaning unverified; return 1 for now
NVN_NOPROTO(nvnTextureGetRawStorageClass, int, const NVNtexture *t) {
    (void)t;
    return 1;
}
#define VW(v) ((view_t *)(v))
NVN(nvnTextureViewSetDefaults, void, NVNtextureView *v) { memset(v, 0, sizeof(view_t)); }
NVN(nvnTextureViewSetLevels, void, NVNtextureView *v, int base, int n) {
    VW(v)->set_levels = 1;
    VW(v)->base_level = base;
    VW(v)->nlevels = n;
}
NVN(nvnTextureViewSetLayers, void, NVNtextureView *v, int min, int n) {
    VW(v)->set_layers = 1;
    VW(v)->min_layer = min;
    VW(v)->nlayers = n;
}
NVN(nvnTextureViewSetFormat, void, NVNtextureView *v, NVNformat f) {
    VW(v)->set_format = 1;
    VW(v)->format = f;
}
NVN(nvnTextureViewSetTarget, void, NVNtextureView *v, NVNtextureTarget t) {
    VW(v)->set_target = 1;
    VW(v)->target = t;
}
NVN(nvnTextureViewGetLayers, NVNboolean, const NVNtextureView *v, int *min, int *n) {
    const view_t *o = (const view_t *)v;
    *min = o->min_layer;
    *n = o->nlayers;
    return o->set_layers;
}
NVN(nvnTextureGetViewOffset, ptrdiff_t, const NVNtexture *t, const NVNtextureView *v) {
    const tex_t *o = CTX(t);
    const view_t *w = (const view_t *)v;
    if (!w)
        return 0;
    uint64_t off = w->set_levels ? nvn_tex_level_offset(&o->d, (unsigned)w->base_level) : 0;
    if (w->set_layers)
        off += (uint64_t)w->min_layer * o->L.layer_size;
    return (ptrdiff_t)off;
}
// CPU 读贴图（NVN ReadTexels）：预排布贴图以池内存为真值（解排布直读），GPU 写出的贴图从 MTLTexture
// 读回（nvn_mtl.m） CPU texture read (NVN ReadTexels): for packaged textures the pool memory is the ground
// truth (de-layout and read directly); GPU-written textures are read back from the MTLTexture (nvn_mtl.m)
static void read_texels(const tex_t *o, const view_t *v, const NVNcopyRegion *r, uint8_t *dst, ptrdiff_t row,
                        ptrdiff_t img) {
    int bw, bh, bpb = nvn_fmt_bpb(o->d.format, &bw, &bh);
    uint32_t wb = (uint32_t)((r->width + bw - 1) / bw) * bpb, hb = (uint32_t)(r->height + bh - 1) / bh;
    if (!row)
        row = wb;
    if (!img)
        img = row * hb;
    unsigned level = v && v->set_levels ? (unsigned)v->base_level : 0,
             layer0 = v && v->set_layers ? (unsigned)v->min_layer : 0;
    x_drain(); // CPU 读贴图：已入队的提交先翻译完 | CPU texture read: submits already enqueued are translated
               // first
    mtl_read_texels(o, level, layer0, r, dst, (size_t)row, (size_t)img);
}
NVN(nvnTextureReadTexels, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r,
    void *dst) {
    read_texels(CTX(t), (const view_t *)v, r, dst, 0, 0);
}
NVN(nvnTextureReadTexelsStrided, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r,
    void *dst, ptrdiff_t row, ptrdiff_t img) {
    read_texels(CTX(t), (const view_t *)v, r, dst, row, img);
}
NVN(nvnTextureInvalidateTexels, void, const NVNtexture *t, const NVNtextureView *v, const NVNcopyRegion *r) {
    (void)t;
    (void)v;
    (void)r;
}

// ---------- 采样器 ----------
// ---------- Samplers ----------
#define SB(b) (&((samp_builder_t *)(b))->p)
NVN(nvnSamplerBuilderSetDefaults, void, NVNsamplerBuilder *b) {
    NVNdevice *dev = ((samp_builder_t *)b)->dev;
    memset(b, 0, sizeof(samp_builder_t));
    ((samp_builder_t *)b)->dev = dev;
    samp_par *o = SB(b);
    o->minf = NVN_MIN_FILTER_NEAREST_MIPMAP_LINEAR;
    o->magf = NVN_MAG_FILTER_LINEAR; // GL 默认 | GL defaults
    o->ws = o->wt = o->wr = NVN_WRAP_MODE_REPEAT;
    o->cmp_func = NVN_COMPARE_FUNC_LESS;
    o->lod_min = -1000.f;
    o->lod_max = 1000.f;
    o->aniso = 1.f;
}
NVN(nvnSamplerBuilderSetDevice, void, NVNsamplerBuilder *b, NVNdevice *d) { ((samp_builder_t *)b)->dev = d; }
NVN(nvnSamplerBuilderSetMinMagFilter, void, NVNsamplerBuilder *b, NVNminFilter mi, NVNmagFilter ma) {
    SB(b)->minf = mi;
    SB(b)->magf = ma;
}
NVN(nvnSamplerBuilderSetWrapMode, void, NVNsamplerBuilder *b, NVNwrapMode s, NVNwrapMode t, NVNwrapMode r) {
    SB(b)->ws = s;
    SB(b)->wt = t;
    SB(b)->wr = r;
}
NVN(nvnSamplerBuilderSetLodClamp, void, NVNsamplerBuilder *b, float lo, float hi) {
    SB(b)->lod_min = lo;
    SB(b)->lod_max = hi;
}
NVN(nvnSamplerBuilderSetLodBias, void, NVNsamplerBuilder *b, float v) { SB(b)->lod_bias = v; }
NVN(nvnSamplerBuilderSetCompare, void, NVNsamplerBuilder *b, NVNcompareMode m, NVNcompareFunc f) {
    SB(b)->cmp_mode = m;
    SB(b)->cmp_func = f;
}
NVN(nvnSamplerBuilderSetBorderColor, void, NVNsamplerBuilder *b, const float *c) {
    memcpy(SB(b)->border, c, 16);
}
NVN(nvnSamplerBuilderSetMaxAnisotropy, void, NVNsamplerBuilder *b, float v) { SB(b)->aniso = v; }
NVN(nvnSamplerBuilderSetLodSnap, void, NVNsamplerBuilder *b, float v) {
    (void)b;
    (void)v;
} // Metal 无对应，忽略 | No Metal equivalent, ignored
NVN(nvnSamplerBuilderGetMinMagFilter, void, const NVNsamplerBuilder *b, NVNminFilter *mi, NVNmagFilter *ma) {
    const samp_par *o = &((const samp_builder_t *)b)->p;
    *mi = (NVNminFilter)o->minf;
    *ma = (NVNmagFilter)o->magf;
}
NVN(nvnSamplerBuilderGetWrapMode, void, const NVNsamplerBuilder *b, NVNwrapMode *s, NVNwrapMode *t,
    NVNwrapMode *r) {
    const samp_par *o = &((const samp_builder_t *)b)->p;
    *s = (NVNwrapMode)o->ws;
    *t = (NVNwrapMode)o->wt;
    *r = (NVNwrapMode)o->wr;
}
NVN(nvnSamplerInitialize, NVNboolean, NVNsampler *s, const NVNsamplerBuilder *b) {
    samp_t *o = (samp_t *)s;
    o->magic = MAGIC('S', 'M', 'P', 0);
    o->p = ((const samp_builder_t *)b)->p;
    return 1;
}
NVN(nvnSamplerFinalize, void, NVNsampler *s) { ((samp_t *)s)->magic = 0; }

// ---------- 程序 ----------
// ---------- Programs ----------
NVN(nvnProgramInitialize, NVNboolean, NVNprogram *p, NVNdevice *d) {
    (void)d;
    memset(p, 0, sizeof(prog_t));
    ((prog_t *)p)->magic = MAGIC('P', 'R', 'O', 'G');
    return 1;
}
NVN(nvnProgramFinalize, void, NVNprogram *p) {
    prog_t *o = (prog_t *)p;
    snap_drop(&o->snap, o->serial, offsetof(prog_t, serial));
    mtl_prog_release(o);
    o->magic = 0;
}
NVN(nvnProgramSetShaders, NVNboolean, NVNprogram *p, int n, const NVNshaderData *sd) {
    prog_t *o = (prog_t *)p;
    if (n < 0 || n > 6) {
        LOG("【红】ProgramSetShaders 阶段数 %d", n);
        return 0;
    }
    snap_drop(&o->snap, o->serial, offsetof(prog_t, serial));
    mtl_prog_release(o);
    o->nstage = n;
    memcpy(o->sd, sd, n * sizeof *sd);
    o->serial = nvn_new_serial();
    OLOG("ProgramSetShaders %p n=%d data0=%#llx", (void *)p, n, n ? (unsigned long long)sd[0].data : 0ull);
    return 1;
}

// ---------- 同步 / 事件 ----------
// ---------- Sync / events ----------
NVN(nvnSyncInitialize, NVNboolean, NVNsync *s, NVNdevice *d) {
    (void)d;
    *(sync_t *)s = (sync_t){MAGIC('S', 'Y', 'N', 'C'), 0, 0};
    return 1;
}
NVN(nvnSyncFinalize, void, NVNsync *s) { ((sync_t *)s)->magic = 0; }
// 围栏 = 「此前提交到队列的命令全部完成」：记下最近一次提交的栅栏值
// Fence = "all commands previously submitted to the queue have completed": record the fence value of the most
// recent submit
NVN(nvnQueueFenceSync, void, NVNqueue *q, NVNsync *s, NVNsyncCondition c, int f) {
    (void)q;
    (void)c;
    (void)f;
    ((sync_t *)s)->fenced = 1;
    ((sync_t *)s)->fence = x_fence();
}
// 没围栏过的同步对象按真机行为等到超时；已围栏的只在 GPU 真正完成到该栅栏值后放行
// A sync object that was never fenced waits until timeout, as on a real device; a fenced one is released only
// after the GPU has actually completed up to that fence value
NVN(nvnSyncWait, NVNsyncWaitResult, const NVNsync *s, uint64_t timeout) {
    const sync_t *o = (const sync_t *)s;
    if (!o->fenced)
        return NVN_SYNC_WAIT_RESULT_TIMEOUT_EXPIRED;
    if (mtl_wait(o->fence, 0))
        return NVN_SYNC_WAIT_RESULT_ALREADY_SIGNALED;
    return mtl_wait(o->fence, timeout) ? NVN_SYNC_WAIT_RESULT_CONDITION_SATISFIED
                                       : NVN_SYNC_WAIT_RESULT_TIMEOUT_EXPIRED;
}
NVN(nvnEventBuilderSetDefaults, void, NVNeventBuilder *b) { memset(b, 0, sizeof(ev_builder_t)); }
NVN(nvnEventBuilderSetStorage, void, NVNeventBuilder *b, const NVNmemoryPool *p, int64_t off) {
    *(ev_builder_t *)b = (ev_builder_t){(const pool_t *)p, off};
}
NVN(nvnEventInitialize, NVNboolean, NVNevent *e, const NVNeventBuilder *b) {
    const ev_builder_t *s = (const ev_builder_t *)b;
    if (!s->pool || !s->pool->mem) {
        LOG("【红】EventInitialize 没有存储");
        return 0;
    }
    *(ev_t *)e = (ev_t){MAGIC('E', 'V', 'N', 'T'), (volatile uint32_t *)(s->pool->mem + s->off)};
    return 1;
}
NVN(nvnEventFinalize, void, NVNevent *e) { ((ev_t *)e)->magic = 0; }
NVN(nvnEventGetValue, uint32_t, const NVNevent *e) { return *((const ev_t *)e)->val; }
static void ev_signal(const ev_t *e, int mode, uint32_t v) {
    if (mode == NVN_EVENT_SIGNAL_MODE_ADD)
        *e->val += v;
    else
        *e->val = v;
}
NVN(nvnEventSignal, void, NVNevent *e, NVNeventSignalMode m, uint32_t v) { ev_signal((const ev_t *)e, m, v); }

// ---------- 队列 ----------
// ---------- Queue ----------
#define QB(b) ((queue_builder_t *)(b))
NVN(nvnQueueBuilderSetDefaults, void, NVNqueueBuilder *b) {
    NVNdevice *dev = QB(b)->dev;
    memset(b, 0, sizeof(queue_builder_t));
    QB(b)->dev = dev;
    QB(b)->cmd_sz = 65536;
    QB(b)->comp_sz = 262144;
    QB(b)->ctl_sz = 16384;
    QB(b)->flush = 4096; // 设备默认值 | Device defaults
}
NVN(nvnQueueBuilderSetDevice, void, NVNqueueBuilder *b, NVNdevice *d) { QB(b)->dev = d; }
NVN(nvnQueueBuilderSetFlags, void, NVNqueueBuilder *b, int f) { QB(b)->flags = f; }
NVN(nvnQueueBuilderSetCommandMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->cmd_sz = n; }
NVN(nvnQueueBuilderSetComputeMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->comp_sz = n; }
NVN(nvnQueueBuilderSetControlMemorySize, void, NVNqueueBuilder *b, size_t n) { QB(b)->ctl_sz = n; }
NVN(nvnQueueBuilderSetCommandFlushThreshold, void, NVNqueueBuilder *b, size_t n) { QB(b)->flush = n; }
// 队列内存：命令 + 计算 + 控制三块之和按 4 KiB 取整（宿主不使用，只需游戏按此大小分配）
// Queue memory: the sum of the command + compute + control blocks rounded to 4 KiB (the host does not use it;
// the game only needs to allocate this size)
NVN(nvnQueueBuilderGetQueueMemorySize, size_t, const NVNqueueBuilder *b) {
    const queue_builder_t *o = (const queue_builder_t *)b;
    return (o->cmd_sz + o->comp_sz + o->ctl_sz + 4095) & ~(size_t)4095;
}
NVN(nvnQueueBuilderSetQueueMemory, void, NVNqueueBuilder *b, void *m, size_t n) {
    QB(b)->mem = m;
    QB(b)->mem_sz = n;
}
NVN(nvnQueueInitialize, NVNboolean, NVNqueue *q, const NVNqueueBuilder *b) {
    *(queue_t *)q = (queue_t){MAGIC('Q', 'U', 'E', 'U'), ((const queue_builder_t *)b)->dev, 0};
    OLOG("QueueInitialize %p", (void *)q);
    return 1;
}
NVN(nvnQueueFinalize, void, NVNqueue *q) { ((queue_t *)q)->magic = 0; }
NVN(nvnQueueGetError, NVNqueueGetErrorResult, NVNqueue *q, NVNqueueErrorInfo *info) {
    (void)q;
    if (info)
        memset(info, 0, sizeof *info);
    return NVN_QUEUE_GET_ERROR_RESULT_GPU_NO_ERROR;
}
NVN(nvnQueueFlush, void, NVNqueue *q) { (void)q; }
NVN(nvnQueueFinish, void, NVNqueue *q) {
    (void)q;
    x_drain();
    mtl_finish();
}

// ---------- 命令缓冲：录进游戏的命令内存，提交时执行 CPU 可见副作用 ----------
// ---------- Command buffer: record into the game's command memory, perform CPU-visible side effects at
// submit ----------
#define CB(c) ((cmd_t *)(c))
#define OP_RESERVE                                                                                           \
    24 // 段尾永远留够一条 JUMP / END | Always keep enough room at the segment end for one JUMP / END

NVN(nvnCommandBufferInitialize, NVNboolean, NVNcommandBuffer *c, NVNdevice *d) {
    memset(c, 0, sizeof(cmd_t));
    CB(c)->magic = MAGIC('C', 'M', 'D', 'B');
    CB(c)->dev = d;
    return 1;
}
NVN(nvnCommandBufferFinalize, void, NVNcommandBuffer *c) { CB(c)->magic = 0; }
NVN(nvnCommandBufferSetMemoryCallback, void, NVNcommandBuffer *c, PFNNVNCOMMANDBUFFERMEMORYCALLBACKPROC cb) {
    CB(c)->cb = cb;
}
NVN(nvnCommandBufferSetMemoryCallbackData, void, NVNcommandBuffer *c, void *d) { CB(c)->cb_data = d; }
static uint8_t *align8(uint8_t *p) { return (uint8_t *)(((uintptr_t)p + 7) & ~(uintptr_t)7); }
// 诊断 DM_NVN_CMDSTAT=1：每 300 帧记一行命令录制的用量——每帧录了多少字节 /
// 多少条、游戏每次给多大一段命令内存、每帧补内存回调几次、宿主补段几次、
// 各类命令的条数与字节（找「我们的记录比真机命令大多少」）。只计数，不改录制 / 回放
// Diagnostic DM_NVN_CMDSTAT=1: every 300 frames log one line of command recording usage -- how many bytes /
// records were recorded per frame, how large a command memory segment the game gives each time, how many
// out-of-memory callbacks per frame, how many host-supplied segments, and the count and bytes of each command
// type (to find "how much larger our records are than real-device commands"). Counts only; does not change
// recording / replay
static struct {
    int on;
    uint64_t bytes, recs, grants, grant_bytes, grant_min, grant_max, cbs, hostseg, op_n[64], op_b[64], frame0,
        waste;
} g_cs = {.on = -1};
static pthread_mutex_t g_cs_mu = PTHREAD_MUTEX_INITIALIZER;
uint64_t dm_nvn_frame_count(void);
static int cs_on(void) {
    if (g_cs.on < 0)
        g_cs.on = getenv("DM_NVN_CMDSTAT") != NULL;
    return g_cs.on;
}
static void cs_op(uint32_t op, uint32_t size) {
    if (!cs_on() || op >= 64)
        return;
    pthread_mutex_lock(&g_cs_mu);
    g_cs.op_n[op]++;
    g_cs.op_b[op] += size;
    pthread_mutex_unlock(&g_cs_mu);
}
static void cs_dump(void) { // 每帧调一次（出图处） | Called once per frame (at present)
    if (!cs_on())
        return;
    uint64_t f = dm_nvn_frame_count();
    if (f - g_cs.frame0 < 300)
        return;
    pthread_mutex_lock(&g_cs_mu);
    double n = (double)(f - g_cs.frame0);
    char top[700];
    int k = 0;
    uint8_t used[64] = {0};
    for (int r = 0; r < 8; r++) {
        int bi = -1;
        for (int i = 0; i < 64; i++)
            if (!used[i] && g_cs.op_b[i] && (bi < 0 || g_cs.op_b[i] > g_cs.op_b[bi]))
                bi = i;
        if (bi < 0)
            break;
        used[bi] = 1;
        k += snprintf(top + k, sizeof top - k, " 命令%d：%.0f 条×%.0f 字节=%.1f KB；", bi, g_cs.op_n[bi] / n,
                      (double)g_cs.op_b[bi] / (double)g_cs.op_n[bi], g_cs.op_b[bi] / n / 1024.0);
    }
    LOG("[nvn] 命令内存 至帧 %llu：每帧录 %.1f KB / %.0f 条；游戏给段 每帧 %.2f 次，每段 均 %.1f KB（最小 "
        "%.1f / 最大 %.1f）；补内存回调 每帧 %.2f 次；宿主补段 每帧 %.3f 次；换段丢弃的段尾 每帧 %.1f "
        "KB｜每帧%s",
        (unsigned long long)f, g_cs.bytes / n / 1024.0, g_cs.recs / n, g_cs.grants / n,
        g_cs.grants ? g_cs.grant_bytes / (double)g_cs.grants / 1024.0 : 0, g_cs.grant_min / 1024.0,
        g_cs.grant_max / 1024.0, g_cs.cbs / n, g_cs.hostseg / n, g_cs.waste / n / 1024.0, top);
    int on = g_cs.on;
    memset(&g_cs, 0, sizeof g_cs);
    g_cs.on = on;
    g_cs.frame0 = f;
    pthread_mutex_unlock(&g_cs_mu);
}
static void put_hdr(uint8_t *p, uint32_t op, uint32_t size) {
    op_hdr h = {op, size};
    memcpy(p, &h, sizeof h);
    if (op)
        cs_op(op, size);
}
// 新段：录制中则在旧段尾写 JUMP 接上
// New segment: if recording is in progress, write a JUMP at the end of the old segment to link it
static void cmd_new_segment(cmd_t *o, uint8_t *mem, size_t n) {
    uint8_t *start = align8(mem), *end = mem + n;
    if (o->recording && o->cur) {
        if (cs_on() && o->end > o->cur) {
            pthread_mutex_lock(&g_cs_mu);
            g_cs.waste += (uint64_t)(o->end - o->cur);
            pthread_mutex_unlock(&g_cs_mu);
        }
        put_hdr(o->cur, OP_JUMP, 16);
        memcpy(o->cur + 8, &start, 8);
    }
    o->cur = start;
    o->end = end;
}
// 游戏自己的命令内存用完时的宿主补段：游戏的补内存回调在它那块命令内存池分不出来时，会拿空池（p = NULL、n =
// 0）调 AddCommandMemory
// （88 级法师档开高级属性页：一帧里列表项很多，原实现在这里读 NULL+8 段错误）。我们的记录比真机 NVN
// 的命令大，游戏按真机用量留的预算不一定够，
// 所以这不是游戏的错误状态而是我们要兜住的：给一段宿主内存接着录。段在提交时就地回放，留 8
// 帧后回收（提交在录制之后同帧或下一帧）。 Host-supplied segment when the game's own command memory runs out:
// when the game's out-of-memory callback cannot allocate from its command memory pool, it calls
// AddCommandMemory with an empty pool (p = NULL, n = 0) (opening the advanced stats page on the level 88
// sorceress save: many list items in one frame, and the original implementation segfaulted here reading
// NULL+8). Our records are larger than real-device NVN commands, so the budget the game reserved for
// real-device usage is not necessarily enough, so this is not an error state of the game but something we
// must catch: provide a segment of host memory to keep recording. The segment is replayed in place at submit
// and reclaimed after being kept for 8 frames (submit is in the same frame as recording or the next one).
#define HOSTSEG_SZ (1u << 20)
#define HOSTSEG_MAX 64
uint64_t dm_nvn_frame_count(void);
static struct {
    uint8_t *mem;
    uint64_t frame;
    int busy;
} g_hostseg[HOSTSEG_MAX];
static pthread_mutex_t g_hostseg_mu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t *hostseg_take(void) {
    static unsigned total;
    uint64_t f = dm_nvn_frame_count();
    uint8_t *m = NULL;
    pthread_mutex_lock(&g_hostseg_mu);
    for (int i = 0; i < HOSTSEG_MAX; i++)
        if (g_hostseg[i].busy && g_hostseg[i].frame + 8 < f)
            g_hostseg[i].busy = 0;
    for (int i = 0; i < HOSTSEG_MAX && !m; i++)
        if (!g_hostseg[i].busy) {
            if (!g_hostseg[i].mem)
                g_hostseg[i].mem = malloc(HOSTSEG_SZ);
            g_hostseg[i].busy = 1;
            g_hostseg[i].frame = f;
            m = g_hostseg[i].mem;
        }
    unsigned t = ++total;
    pthread_mutex_unlock(&g_hostseg_mu);
    if (!m)
        die("NVN 命令内存：游戏池已空，宿主补段 %d 段也用完（8 帧内）", HOSTSEG_MAX);
    if (t == 1 || t % 256 == 0)
        LOG("[nvn] 游戏的命令内存池已空，宿主补段（累计 %u 次，帧 %llu）", t, (unsigned long long)f);
    return m;
}
NVN(nvnCommandBufferAddCommandMemory, void, NVNcommandBuffer *c, const NVNmemoryPool *p, ptrdiff_t off,
    size_t n) {
    const pool_t *pl = (const pool_t *)p;
    if (!pl || n < 64) {
        if (cs_on()) {
            pthread_mutex_lock(&g_cs_mu);
            g_cs.hostseg++;
            pthread_mutex_unlock(&g_cs_mu);
        }
        cmd_new_segment(CB(c), hostseg_take(), HOSTSEG_SZ);
        return;
    }
    if (!pl->mem) {
        LOG("【红】AddCommandMemory 池没有 CPU 存储");
        return;
    }
    if (cs_on()) {
        pthread_mutex_lock(&g_cs_mu);
        g_cs.grants++;
        g_cs.grant_bytes += n;
        if (!g_cs.grant_min || n < g_cs.grant_min)
            g_cs.grant_min = n;
        if (n > g_cs.grant_max)
            g_cs.grant_max = n;
        pthread_mutex_unlock(&g_cs_mu);
    }
    cmd_new_segment(CB(c), pl->mem + off, n);
}
NVN(nvnCommandBufferAddControlMemory, void, NVNcommandBuffer *c, void *m, size_t n) {
    (void)c;
    (void)m;
    (void)n;
}
// 取一条记录的空间；不够则按 NVN 约定回调游戏补命令内存
// Get space for one record; if not enough, call back into the game per the NVN convention to add command
// memory
static uint8_t *cmd_alloc(cmd_t *o, uint32_t size) {
    size = (size + 7) & ~7u;
    for (int tries = 0; !o->cur || o->cur + size + OP_RESERVE > o->end; tries++) {
        if (!o->cb || tries == 2) {
            cmd_new_segment(o, hostseg_take(), HOSTSEG_SZ);
            continue;
        }
        if (cs_on()) {
            pthread_mutex_lock(&g_cs_mu);
            g_cs.cbs++;
            pthread_mutex_unlock(&g_cs_mu);
        }
        o->cb((NVNcommandBuffer *)o, NVN_COMMAND_BUFFER_MEMORY_EVENT_OUT_OF_COMMAND_MEMORY, size + OP_RESERVE,
              o->cb_data);
    }
    if (cs_on()) {
        pthread_mutex_lock(&g_cs_mu);
        g_cs.bytes += size;
        g_cs.recs++;
        pthread_mutex_unlock(&g_cs_mu);
    }
    uint8_t *p = o->cur;
    o->cur += size;
    put_hdr(p, 0, size);
    return p;
}
NVN(nvnCommandBufferBeginRecording, void, NVNcommandBuffer *c) {
    cmd_t *o = CB(c);
    if (o->recording)
        LOG("【红】BeginRecording 重入");
    o->recording = 1;
    uint8_t *p = cmd_alloc(o, 8);
    put_hdr(p, OP_BEGIN, 8);
    o->begin = p;
}
NVN(nvnCommandBufferEndRecording, NVNcommandHandle, NVNcommandBuffer *c) {
    cmd_t *o = CB(c);
    uint8_t *p = cmd_alloc(o, 8);
    put_hdr(p, OP_END, 8);
    o->recording = 0;
    return (NVNcommandHandle)o->begin;
}
NVN(nvnCommandBufferIsRecording, NVNboolean, const NVNcommandBuffer *c) {
    return ((const cmd_t *)c)->recording != 0;
}
NVN(nvnCommandBufferCopyCommands, void, NVNcommandBuffer *c, int n, const NVNcommandHandle *h) {
    for (int i = 0; i < n; i++) {
        uint8_t *p = cmd_alloc(CB(c), 16);
        put_hdr(p, OP_CALL, 16);
        memcpy(p + 8, &h[i], 8);
    }
}
NVN(nvnCommandBufferSignalEvent, void, NVNcommandBuffer *c, const NVNevent *e, NVNeventSignalMode m,
    NVNeventSignalLocation loc, int flags, uint32_t v) {
    (void)loc;
    (void)flags;
    uint8_t *p = cmd_alloc(CB(c), 24);
    put_hdr(p, OP_SIGNAL_EVENT, 24);
    const ev_t *ev = (const ev_t *)e;
    volatile uint32_t *val = ev->val;
    uint32_t mm = m;
    memcpy(p + 8, &val, 8);
    memcpy(p + 16, &mm, 4);
    memcpy(p + 20, &v, 4);
}
NVN(nvnCommandBufferResetCounter, void, NVNcommandBuffer *c, NVNcounterType t) {
    uint8_t *p = cmd_alloc(CB(c), 16);
    put_hdr(p, OP_RESET_COUNTER, 16);
    uint64_t tt = t;
    memcpy(p + 8, &tt, 8);
}
NVN(nvnCommandBufferReportCounter, void, NVNcommandBuffer *c, NVNcounterType t, NVNbufferAddress a) {
    uint8_t *p = cmd_alloc(CB(c), 24);
    put_hdr(p, OP_REPORT_COUNTER, 24);
    uint64_t tt = t;
    memcpy(p + 8, &tt, 8);
    memcpy(p + 16, &a, 8);
}
NVN(nvnCommandBufferCopyBufferToBuffer, void, NVNcommandBuffer *c, NVNbufferAddress src, NVNbufferAddress dst,
    size_t n, int flags) {
    (void)flags;
    uint8_t *p = cmd_alloc(CB(c), 32);
    put_hdr(p, OP_COPY_B2B, 32);
    uint64_t nn = n;
    memcpy(p + 8, &src, 8);
    memcpy(p + 16, &dst, 8);
    memcpy(p + 24, &nn, 8);
}
// uniform 更新：数据在录制时拷进命令内存（NVN 语义：录制后调用方可改原数据），提交时写进缓冲
// uniform update: the data is copied into command memory at record time (NVN semantics: the caller may modify
// the original data after recording) and written into the buffer at submit
NVN(nvnCommandBufferUpdateUniformBuffer, void, NVNcommandBuffer *c, NVNbufferAddress buf, size_t bufsz,
    ptrdiff_t off, size_t n, const void *data) {
    (void)bufsz;
    uint32_t sz = (uint32_t)(24 + n);
    uint8_t *p = cmd_alloc(CB(c), sz);
    put_hdr(p, OP_UPDATE_UB, (sz + 7) & ~7u);
    uint64_t dst = buf + off, nn = n;
    memcpy(p + 8, &dst, 8);
    memcpy(p + 16, &nn, 8);
    memcpy(p + 24, data, n);
}

// ---------- 渲染命令：参数按 nvn_obj.h 的载荷格式录进命令内存，回放见 nvn_mtl.m ----------
// ---------- Render commands: parameters are recorded into command memory in the payload format of nvn_obj.h;
// for replay see nvn_mtl.m ----------
static void *rec(NVNcommandBuffer *c, uint32_t op, size_t payload) {
    uint32_t sz = (uint32_t)((8 + payload + 7) & ~(size_t)7);
    uint8_t *p = cmd_alloc(CB(c), sz);
    put_hdr(p, op, sz);
    memset(p + 8, 0, sz - 8);
    return p + 8;
}
#define REC(c, OP, T) ((T *)rec((c), (OP), sizeof(T)))
// 带数组的绑定记录按实际个数分配：前缀字段 + n 个元素（回放端只读前 n 个）。原先按最大个数定长记（绑顶点缓冲
// 272 字节、绑贴图 / 采样器 280 字节， 实际常只有 1–2 个），空尾占每帧命令字节的四到五成；88
// 级法师的高级属性页每帧录 12.5 MB，把游戏自己的命令内存池（约每帧 105 段 × 128 KB）用空 Binding records with
// arrays are allocated by the actual count: prefix fields + n elements (the replay side reads only the first
// n). Previously they were recorded at fixed length for the maximum count (bind vertex buffers 272 bytes,
// bind textures / samplers 280 bytes, while there are usually only 1–2 in practice), and the empty tail took
// 40–50% of the command bytes per frame; the advanced stats page of the level 88 sorceress recorded 12.5 MB
// per frame, using up the game's own command memory pool (about 105 segments x 128 KB per frame)
#define RECN(c, OP, T, ARR, N)                                                                               \
    ((T *)rec((c), (OP), offsetof(T, ARR) + sizeof(((T *)0)->ARR[0]) * (size_t)(N)))
static void copy_view(view_t *dst, uint8_t *has, const NVNtextureView *v) {
    *has = v != NULL;
    if (v)
        *dst = *(const view_t *)v;
}
NVN(nvnCommandBufferSetRenderTargets, void, NVNcommandBuffer *c, int n, const NVNtexture *const *ct,
    const NVNtextureView *const *cv, const NVNtexture *dt, const NVNtextureView *dv) {
    if (n < 0 || n > 8) {
        LOG("【红】SetRenderTargets 数量 %d", n);
        n = n < 0 ? 0 : 8;
    }
    p_set_rt *r = REC(c, OP_SET_RT, p_set_rt);
    r->n = n;
    for (int i = 0; i < n; i++) {
        r->color[i] = ct ? snap_tex(ct[i]) : NULL;
        copy_view(&r->cview[i], &r->has_cview[i], cv ? cv[i] : NULL);
    }
    r->depth = snap_tex(dt);
    copy_view(&r->dview, &r->has_dview, dv);
}
NVN(nvnCommandBufferClearColor, void, NVNcommandBuffer *c, int index, const float *rgba, int mask) {
    p_clear_color *r = REC(c, OP_CLEAR_COLOR, p_clear_color);
    r->index = index;
    r->mask = mask;
    memcpy(r->rgba, rgba, 16);
}
NVN(nvnCommandBufferClearDepthStencil, void, NVNcommandBuffer *c, float depth, uint32_t dmask, int stencil,
    int smask) {
    p_clear_ds *r = REC(c, OP_CLEAR_DS, p_clear_ds);
    *r = (p_clear_ds){depth, (int)B(dmask), stencil, smask};
}
NVN(nvnCommandBufferSetViewports, void, NVNcommandBuffer *c, int first, int n, const float *v) {
    if (n < 0 || first + n > 16) {
        LOG("【红】SetViewports first=%d n=%d", first, n);
        return;
    }
    p_viewports *r = REC(c, OP_VIEWPORTS, p_viewports);
    r->first = first;
    r->n = n;
    for (int i = 0; i < n; i++)
        memcpy(r->v[i], v + 4 * i, 16);
}
NVN(nvnCommandBufferSetScissor, void, NVNcommandBuffer *c, int x, int y, int w, int h) {
    *REC(c, OP_SCISSOR, p_scissor) = (p_scissor){x, y, w, h};
}
NVN(nvnCommandBufferSetDepthRange, void, NVNcommandBuffer *c, float n, float f) {
    *REC(c, OP_DEPTH_RANGE, p_depth_range) = (p_depth_range){n, f};
}
NVN(nvnCommandBufferBindProgram, void, NVNcommandBuffer *c, const NVNprogram *p, int stages) {
    *REC(c, OP_BIND_PROGRAM, p_bind_program) = (p_bind_program){snap_prog(p), stages};
}
static void bind_state(NVNcommandBuffer *c, int kind, const void *st, size_t n) {
    p_bind_state *r = REC(c, OP_BIND_STATE, p_bind_state);
    r->kind = kind;
    memcpy(r->bytes, st, n);
}
NVN(nvnCommandBufferBindBlendState, void, NVNcommandBuffer *c, const NVNblendState *st) {
    bind_state(c, 0, st, sizeof(blend_st));
}
NVN(nvnCommandBufferBindColorState, void, NVNcommandBuffer *c, const NVNcolorState *st) {
    bind_state(c, 1, st, sizeof(color_st));
}
NVN(nvnCommandBufferBindChannelMaskState, void, NVNcommandBuffer *c, const NVNchannelMaskState *st) {
    bind_state(c, 2, st, sizeof(chmask_st));
}
NVN(nvnCommandBufferBindMultisampleState, void, NVNcommandBuffer *c, const NVNmultisampleState *st) {
    bind_state(c, 3, st, sizeof(ms_st));
}
NVN(nvnCommandBufferBindPolygonState, void, NVNcommandBuffer *c, const NVNpolygonState *st) {
    bind_state(c, 4, st, sizeof(poly_st));
}
NVN(nvnCommandBufferBindDepthStencilState, void, NVNcommandBuffer *c, const NVNdepthStencilState *st) {
    bind_state(c, 5, st, sizeof(ds_st));
}
NVN(nvnCommandBufferBindVertexAttribState, void, NVNcommandBuffer *c, int n, const NVNvertexAttribState *st) {
    if (n < 0 || n > 16) {
        LOG("【红】BindVertexAttribState n=%d", n);
        return;
    }
    p_bind_vattr *r = RECN(c, OP_BIND_VATTR, p_bind_vattr, st, n);
    r->first = 0;
    r->n = n;
    memcpy(r->st, st, 4 * n);
}
NVN(nvnCommandBufferBindVertexStreamState, void, NVNcommandBuffer *c, int n, const NVNvertexStreamState *st) {
    if (n < 0 || n > 16) {
        LOG("【红】BindVertexStreamState n=%d", n);
        return;
    }
    p_bind_vstream *r = RECN(c, OP_BIND_VSTREAM, p_bind_vstream, st, n);
    r->first = 0;
    r->n = n;
    memcpy(r->st, st, 8 * n);
}
NVN(nvnCommandBufferBindVertexBuffers, void, NVNcommandBuffer *c, int first, int n,
    const NVNbufferRange *rg) {
    if (n < 0 || first + n > 16) {
        LOG("【红】BindVertexBuffers first=%d n=%d", first, n);
        return;
    }
    p_bind_vbuf *r = RECN(c, OP_BIND_VBUF, p_bind_vbuf, r, n);
    r->first = first;
    r->n = n;
    memcpy(r->r, rg, sizeof *rg * n);
}
NVN(nvnCommandBufferBindUniformBuffer, void, NVNcommandBuffer *c, NVNshaderStage st, int i,
    NVNbufferAddress a, size_t n) {
    *REC(c, OP_BIND_UBO, p_bind_buf) = (p_bind_buf){st, i, a, n};
}
NVN(nvnCommandBufferBindUniformBuffers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n,
    const NVNbufferRange *rg) {
    for (int i = 0; i < n; i++)
        *REC(c, OP_BIND_UBO, p_bind_buf) = (p_bind_buf){st, first + i, rg[i].address, rg[i].size};
}
NVN(nvnCommandBufferBindStorageBuffer, void, NVNcommandBuffer *c, NVNshaderStage st, int i,
    NVNbufferAddress a, size_t n) {
    *REC(c, OP_BIND_SSBO, p_bind_buf) = (p_bind_buf){st, i, a, n};
}
NVN(nvnCommandBufferBindStorageBuffers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n,
    const NVNbufferRange *rg) {
    for (int i = 0; i < n; i++)
        *REC(c, OP_BIND_SSBO, p_bind_buf) = (p_bind_buf){st, first + i, rg[i].address, rg[i].size};
}
static void bind_handles(NVNcommandBuffer *c, int op, int st, int first, int n, const uint64_t *h) {
    if (n < 0 || n > 32) {
        LOG("【红】绑定句柄数 %d", n);
        return;
    }
    p_bind_handles *r = RECN(c, op, p_bind_handles, h, n);
    r->stage = st;
    r->first = first;
    r->n = n;
    memcpy(r->h, h, 8 * n);
}
// 图像（可写贴图）：句柄 = 贴图池号（GetImageHandle 原样返回；游戏不调 RegisterImage，图像与贴图共用登记）
// Image (writable texture): handle = texture pool index (GetImageHandle returns it as-is; the game does not
// call RegisterImage, images and textures share registration)
NVN(nvnCommandBufferBindImages, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n,
    const NVNimageHandle *h) {
    bind_handles(c, OP_BIND_IMG, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateTextures, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n,
    const NVNseparateTextureHandle *h) {
    bind_handles(c, OP_BIND_TEX, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateSamplers, void, NVNcommandBuffer *c, NVNshaderStage st, int first, int n,
    const NVNseparateSamplerHandle *h) {
    bind_handles(c, OP_BIND_SAMP, st, first, n, (const uint64_t *)h);
}
NVN(nvnCommandBufferBindSeparateSampler, void, NVNcommandBuffer *c, NVNshaderStage st, int i,
    NVNseparateSamplerHandle h) {
    bind_handles(c, OP_BIND_SAMP, st, i, 1, &h.value);
}
NVN(nvnCommandBufferSetStencilValueMask, void, NVNcommandBuffer *c, NVNface f, int m) {
    *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, m, 0, 0, 0};
}
NVN(nvnCommandBufferSetStencilMask, void, NVNcommandBuffer *c, NVNface f, int m) {
    *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, 0, m, 0, 1};
}
NVN(nvnCommandBufferSetStencilRef, void, NVNcommandBuffer *c, NVNface f, int v) {
    *REC(c, OP_STENCIL, p_stencil) = (p_stencil){f, 0, 0, v, 2};
}
NVN(nvnCommandBufferSetPolygonOffsetClamp, void, NVNcommandBuffer *c, float fa, float un, float cl) {
    *REC(c, OP_POLY_OFFSET, p_poly_offset) = (p_poly_offset){fa, un, cl};
}
NVN(nvnCommandBufferDrawArrays, void, NVNcommandBuffer *c, NVNdrawPrimitive pr, int first, int n) {
    *REC(c, OP_DRAW, p_draw) = (p_draw){pr, first, n};
}
NVN(nvnCommandBufferDrawElementsInstanced, void, NVNcommandBuffer *c, NVNdrawPrimitive pr, NVNindexType it,
    int n, NVNbufferAddress ib, int bv, int bi, int inst) {
    *REC(c, OP_DRAW_IDX, p_draw_idx) = (p_draw_idx){pr, it, n, bv, bi, inst, ib};
}
NVN(nvnCommandBufferDispatchCompute, void, NVNcommandBuffer *c, int x, int y, int z) {
    // 跳板是尾跳（br），这里的返回地址就是游戏里的调用点
    // The thunk is a tail jump (br), so the return address here is the call site in the game
    *REC(c, OP_DISPATCH, p_dispatch) = (p_dispatch){x, y, z, (uint64_t)__builtin_return_address(0)};
}
NVN(nvnCommandBufferBarrier, void, NVNcommandBuffer *c, int bits) {
    *(int32_t *)rec(c, OP_BARRIER, 8) = bits;
}
NVN(nvnCommandBufferSetTexturePool, void, NVNcommandBuffer *c, const NVNtexturePool *p) {
    REC(c, OP_SET_TEX_POOL, p_set_pool)->pool = (const dpool_t *)p;
}
NVN(nvnCommandBufferSetSamplerPool, void, NVNcommandBuffer *c, const NVNsamplerPool *p) {
    REC(c, OP_SET_SAMP_POOL, p_set_pool)->pool = (const dpool_t *)p;
}

NVN(nvnCommandBufferCopyBufferToTexture, void, NVNcommandBuffer *c, NVNbufferAddress src, const NVNtexture *t,
    const NVNtextureView *v, const NVNcopyRegion *r, int flags) {
    p_copy_b2t *o = REC(c, OP_COPY_B2T, p_copy_b2t);
    o->src = src;
    o->tex = snap_tex(t);
    copy_view(&o->view, &o->has_view, v);
    o->r = *r;
    o->flags = flags;
}
NVN(nvnCommandBufferCopyTextureToBuffer, void, NVNcommandBuffer *c, const NVNtexture *t,
    const NVNtextureView *v, const NVNcopyRegion *r, NVNbufferAddress dst, int flags) {
    p_copy_b2t *o = REC(c, OP_COPY_T2B, p_copy_b2t);
    o->src = dst;
    o->tex = snap_tex(t);
    copy_view(&o->view, &o->has_view, v);
    o->r = *r;
    o->flags = flags;
}
// 用给定值清贴图区域（浮点 / 无符号整数两版；mask = 通道写掩码）
// Clear a texture region with the given value (float / unsigned integer versions; mask = channel write mask)
NVN(nvnCommandBufferClearTexture, void, NVNcommandBuffer *c, const NVNtexture *t, const NVNtextureView *v,
    const NVNcopyRegion *r, const float *rgba, int mask) {
    p_clear_tex *o = REC(c, OP_CLEAR_TEX, p_clear_tex);
    o->tex = snap_tex(t);
    copy_view(&o->view, &o->has_view, v);
    o->r = *r;
    o->mask = mask;
    o->is_uint = 0;
    memcpy(o->c.f, rgba, 16);
}
NVN(nvnCommandBufferClearTextureui, void, NVNcommandBuffer *c, const NVNtexture *t, const NVNtextureView *v,
    const NVNcopyRegion *r, const uint32_t *rgba, int mask) {
    p_clear_tex *o = REC(c, OP_CLEAR_TEX, p_clear_tex);
    o->tex = snap_tex(t);
    copy_view(&o->view, &o->has_view, v);
    o->r = *r;
    o->mask = mask;
    o->is_uint = 1;
    memcpy(o->c.u, rgba, 16);
}
NVN(nvnCommandBufferCopyTextureToTexture, void, NVNcommandBuffer *c, const NVNtexture *s,
    const NVNtextureView *sv, const NVNcopyRegion *sr, const NVNtexture *d, const NVNtextureView *dv,
    const NVNcopyRegion *dr, int flags) {
    p_copy_t2t *o = REC(c, OP_COPY_T2T, p_copy_t2t);
    o->src = snap_tex(s);
    o->dst = snap_tex(d);
    copy_view(&o->sv, &o->has_sv, sv);
    copy_view(&o->dv, &o->has_dv, dv);
    o->sr = *sr;
    o->dr = *dr;
    o->flags = flags;
}
NVN(nvnCommandBufferSetCopyRowStride, void, NVNcommandBuffer *c, ptrdiff_t st) {
    REC(c, OP_COPY_STRIDE, p_copy_stride)->stride = st;
}
NVN(nvnCommandBufferSetCopyImageStride, void, NVNcommandBuffer *c, ptrdiff_t st) {
    REC(c, OP_COPY_IMAGE_STRIDE, p_copy_stride)->stride = st;
}
NVN(nvnCommandBufferSetBlendColor, void, NVNcommandBuffer *c, const float *v) {
    memcpy(REC(c, OP_BLEND_COLOR, p_blend_color)->rgba, v, 16);
}

// ---------- 状态对象：就地打包（nvn_obj.h） ----------
// ---------- State objects: packed in place (nvn_obj.h) ----------
#define ST(T, x) ((T *)(x))
NVN(nvnBlendStateSetDefaults, void, NVNblendState *s) {
    *ST(blend_st, s) = (blend_st){0,
                                  NVN_BLEND_FUNC_ONE,
                                  NVN_BLEND_FUNC_ZERO,
                                  NVN_BLEND_FUNC_ONE,
                                  NVN_BLEND_FUNC_ZERO,
                                  NVN_BLEND_EQUATION_ADD,
                                  NVN_BLEND_EQUATION_ADD};
}
NVN(nvnBlendStateSetBlendTarget, void, NVNblendState *s, int t) { ST(blend_st, s)->target = t; }
NVN(nvnBlendStateSetBlendFunc, void, NVNblendState *s, NVNblendFunc sr, NVNblendFunc dr, NVNblendFunc sa,
    NVNblendFunc da) {
    blend_st *o = ST(blend_st, s);
    o->src_rgb = sr;
    o->dst_rgb = dr;
    o->src_a = sa;
    o->dst_a = da;
}
NVN(nvnBlendStateSetBlendEquation, void, NVNblendState *s, NVNblendEquation er, NVNblendEquation ea) {
    ST(blend_st, s)->eq_rgb = er;
    ST(blend_st, s)->eq_a = ea;
}
NVN(nvnColorStateSetDefaults, void, NVNcolorState *s) {
    *ST(color_st, s) = (color_st){0, NVN_LOGIC_OP_COPY, NVN_ALPHA_FUNC_ALWAYS};
}
NVN(nvnColorStateSetBlendEnable, void, NVNcolorState *s, int i, uint32_t en) {
    color_st *o = ST(color_st, s);
    if (i < 0 || i > 7)
        return;
    o->blend_en = B(en) ? (o->blend_en | 1u << i) : (o->blend_en & ~(1u << i));
}
NVN(nvnChannelMaskStateSetDefaults, void, NVNchannelMaskState *s) { ST(chmask_st, s)->mask = 0xffffffffu; }
NVN(nvnChannelMaskStateSetChannelMask, void, NVNchannelMaskState *s, int i, uint32_t r, uint32_t g,
    uint32_t b, uint32_t a) {
    if (i < 0 || i > 7)
        return;
    uint32_t m = (B(r) ? 1 : 0) | (B(g) ? 2 : 0) | (B(b) ? 4 : 0) | (B(a) ? 8 : 0);
    chmask_st *o = ST(chmask_st, s);
    o->mask = (o->mask & ~(0xfu << (4 * i))) | m << (4 * i);
}
NVN(nvnMultisampleStateSetDefaults, void, NVNmultisampleState *s) {
    memset(s, 0, sizeof(ms_st));
    ST(ms_st, s)->enable = 1;
}
NVN(nvnMultisampleStateSetMultisampleEnable, void, NVNmultisampleState *s, uint32_t e) {
    ST(ms_st, s)->enable = B(e) != 0;
}
NVN(nvnMultisampleStateSetSamples, void, NVNmultisampleState *s, int n) {
    ST(ms_st, s)->samples = (uint8_t)n;
}
NVN(nvnMultisampleStateSetAlphaToCoverageEnable, void, NVNmultisampleState *s, uint32_t e) {
    ST(ms_st, s)->a2c = B(e) != 0;
}
NVN(nvnMultisampleStateSetSampleLocationsEnable, void, NVNmultisampleState *s, uint32_t e) {
    ST(ms_st, s)->sl_en = B(e) != 0;
}
NVN(nvnMultisampleStateSetSampleLocations, void, NVNmultisampleState *s, int start, int n, const float *v) {
    (void)s;
    (void)start;
    (void)n;
    (void)v;
}
NVN(nvnPolygonStateSetDefaults, void, NVNpolygonState *s) {
    *ST(poly_st, s) = (poly_st){NVN_FACE_NONE, NVN_FRONT_FACE_CCW, NVN_POLYGON_MODE_FILL, 0};
}
NVN(nvnPolygonStateSetCullFace, void, NVNpolygonState *s, NVNface f) { ST(poly_st, s)->cull = f; }
NVN(nvnPolygonStateSetFrontFace, void, NVNpolygonState *s, NVNfrontFace f) { ST(poly_st, s)->front = f; }
NVN(nvnPolygonStateSetPolygonMode, void, NVNpolygonState *s, NVNpolygonMode m) { ST(poly_st, s)->mode = m; }
NVN(nvnPolygonStateSetPolygonOffsetEnables, void, NVNpolygonState *s, int e) {
    ST(poly_st, s)->offset_en = e;
}
NVN(nvnDepthStencilStateSetDefaults, void, NVNdepthStencilState *s) {
    *ST(ds_st, s) = (ds_st){0,
                            1,
                            NVN_DEPTH_FUNC_LESS,
                            0,
                            NVN_STENCIL_FUNC_ALWAYS,
                            NVN_STENCIL_FUNC_ALWAYS,
                            NVN_STENCIL_OP_KEEP,
                            NVN_STENCIL_OP_KEEP,
                            NVN_STENCIL_OP_KEEP,
                            NVN_STENCIL_OP_KEEP,
                            NVN_STENCIL_OP_KEEP,
                            NVN_STENCIL_OP_KEEP};
}
NVN(nvnDepthStencilStateSetDepthTestEnable, void, NVNdepthStencilState *s, uint32_t e) {
    ST(ds_st, s)->dt = B(e) != 0;
}
NVN(nvnDepthStencilStateSetDepthWriteEnable, void, NVNdepthStencilState *s, uint32_t e) {
    ST(ds_st, s)->dw = B(e) != 0;
}
NVN(nvnDepthStencilStateSetDepthFunc, void, NVNdepthStencilState *s, NVNdepthFunc f) { ST(ds_st, s)->df = f; }
NVN(nvnDepthStencilStateSetStencilTestEnable, void, NVNdepthStencilState *s, uint32_t e) {
    ST(ds_st, s)->st = B(e) != 0;
}
NVN(nvnDepthStencilStateSetStencilFunc, void, NVNdepthStencilState *s, NVNface f, NVNstencilFunc fn) {
    ds_st *o = ST(ds_st, s);
    if (f & NVN_FACE_FRONT)
        o->sf_f = fn;
    if (f & NVN_FACE_BACK)
        o->sf_b = fn;
}
NVN(nvnDepthStencilStateSetStencilOp, void, NVNdepthStencilState *s, NVNface f, NVNstencilOp fail,
    NVNstencilOp zfail, NVNstencilOp zpass) {
    ds_st *o = ST(ds_st, s);
    if (f & NVN_FACE_FRONT) {
        o->fail_f = fail;
        o->zfail_f = zfail;
        o->zpass_f = zpass;
    }
    if (f & NVN_FACE_BACK) {
        o->fail_b = fail;
        o->zfail_b = zfail;
        o->zpass_b = zpass;
    }
}
NVN(nvnVertexAttribStateSetDefaults, void, NVNvertexAttribState *s) { *ST(va_st, s) = (va_st){0, 0, 0}; }
NVN(nvnVertexAttribStateSetFormat, void, NVNvertexAttribState *s, NVNformat f, ptrdiff_t off) {
    if (off < 0 || off >= (1 << 19))
        LOG("【红】顶点属性偏移 %td 超出打包范围", off);
    ST(va_st, s)->fmt = f;
    ST(va_st, s)->offset = (uint32_t)off;
}
NVN(nvnVertexAttribStateSetStreamIndex, void, NVNvertexAttribState *s, int i) { ST(va_st, s)->stream = i; }
NVN(nvnVertexStreamStateSetDefaults, void, NVNvertexStreamState *s) { *ST(vs_st, s) = (vs_st){0, 0}; }
NVN(nvnVertexStreamStateSetStride, void, NVNvertexStreamState *s, ptrdiff_t st) {
    ST(vs_st, s)->stride = (uint32_t)st;
}
NVN(nvnVertexStreamStateSetDivisor, void, NVNvertexStreamState *s, int d) { ST(vs_st, s)->divisor = d; }
NVN(nvnMemoryPoolMap, void *, const NVNmemoryPool *p) { return ((const pool_t *)p)->mem; }
NVN(nvnMemoryPoolGetSize, size_t, const NVNmemoryPool *p) { return ((const pool_t *)p)->size; }
NVN(nvnMemoryPoolGetFlags, NVNmemoryPoolFlags, const NVNmemoryPool *p) {
    return (NVNmemoryPoolFlags)((const pool_t *)p)->flags;
}
NVN(nvnTextureBuilderGetWidth, int, const NVNtextureBuilder *b) { return TB(b)->d.width; }
NVN(nvnTextureBuilderGetHeight, int, const NVNtextureBuilder *b) { return TB(b)->d.height; }
NVN(nvnTextureBuilderGetDepth, int, const NVNtextureBuilder *b) { return TB(b)->d.depth; }
NVN(nvnTextureBuilderGetLevels, int, const NVNtextureBuilder *b) { return TB(b)->d.levels; }
NVN(nvnTextureBuilderGetFormat, NVNformat, const NVNtextureBuilder *b) { return (NVNformat)TB(b)->d.format; }
NVN(nvnTextureBuilderGetTarget, NVNtextureTarget, const NVNtextureBuilder *b) {
    return (NVNtextureTarget)TB(b)->d.target;
}
NVN(nvnTextureBuilderSetSize2D, void, NVNtextureBuilder *b, int w, int h) {
    TB(b)->d.width = w;
    TB(b)->d.height = h;
    TB(b)->d.depth = 1;
}
NVN(nvnTextureBuilderSetSize3D, void, NVNtextureBuilder *b, int w, int h, int d) {
    TB(b)->d.width = w;
    TB(b)->d.height = h;
    TB(b)->d.depth = d;
}
NVN(nvnTextureGetFormat, NVNformat, const NVNtexture *t) { return (NVNformat)CTX(t)->d.format; }
NVN(nvnDeviceGetTextureHandle, NVNtextureHandle, const NVNdevice *d, int t, int s) {
    (void)d;
    return (uint64_t)t | ((uint64_t)s << 20);
}
NVN(nvnDeviceGetTexelFetchHandle, NVNtextureHandle, const NVNdevice *d, int t) {
    (void)d;
    return t;
}
NVN(nvnCommandBufferCallCommands, void, NVNcommandBuffer *c, int n, const NVNcommandHandle *h) {
    h_nvnCommandBufferCopyCommands(c, n, h);
}
NVN(nvnCommandBufferDrawElements, void, NVNcommandBuffer *c, NVNdrawPrimitive p, NVNindexType t, int n,
    NVNbufferAddress b) {
    *REC(c, OP_DRAW_IDX, p_draw_idx) = (p_draw_idx){p, t, n, 0, 0, 1, b};
}
NVN(nvnQueueSubmitCommands, void, NVNqueue *q, int n, const NVNcommandHandle *h) {
    (void)q;
    uint64_t k = ++g_x_subs;
    dmReplaySubmit(n, h);
    snap_tick(k);
}
NVN(nvnWindowBuilderSetDefaults, void, NVNwindowBuilder *b) {
    memset(b, 0, sizeof(win_builder_t));
    ((win_builder_t *)b)->ntex = 2;
    ((win_builder_t *)b)->interval = 1;
}
NVN(nvnWindowBuilderSetDevice, void, NVNwindowBuilder *b, NVNdevice *d) { ((win_builder_t *)b)->dev = d; }
NVN(nvnWindowBuilderSetNativeWindow, void, NVNwindowBuilder *b, void *w) { ((win_builder_t *)b)->native = w; }
NVN(nvnWindowBuilderSetPresentInterval, void, NVNwindowBuilder *b, int i) {
    ((win_builder_t *)b)->interval = i;
}
NVN(nvnWindowBuilderSetTextures, void, NVNwindowBuilder *b, int n, NVNtexture *const *t) {
    win_builder_t *o = (win_builder_t *)b;
    o->ntex = n > 4 ? 4 : n;
    for (int i = 0; i < o->ntex; i++)
        o->tex[i] = t[i];
}
NVN(nvnWindowInitialize, NVNboolean, NVNwindow *w, const NVNwindowBuilder *b) {
    const win_builder_t *s = (const win_builder_t *)b;
    win_t *o = (win_t *)w;
    memset(o, 0, sizeof *o);
    o->native = s->native;
    o->ntex = s->ntex;
    o->interval = s->interval;
    memcpy(o->tex, s->tex, sizeof o->tex);
    return 1;
}
NVN(nvnWindowAcquireTexture, NVNwindowAcquireTextureResult, NVNwindow *w, NVNsync *s, int *out) {
    win_t *o = (win_t *)w;
    *out = o->next++ % (o->ntex ? o->ntex : 2);
    if (s) {
        sync_t *z = (sync_t *)s;
        z->fenced = 1;
        z->fence = dmReplaySerial();
    }
    return NVN_WINDOW_ACQUIRE_TEXTURE_RESULT_SUCCESS;
}
NVN(nvnQueueAcquireTexture, NVNqueueAcquireTextureResult, NVNqueue *q, NVNwindow *w, int *out) {
    (void)q;
    (void)h_nvnWindowAcquireTexture(w, NULL, out);
    return NVN_QUEUE_ACQUIRE_TEXTURE_RESULT_SUCCESS;
}
NVN(nvnQueuePresentTexture, void, NVNqueue *q, NVNwindow *w, int i) {
    (void)q;
    win_t *o = (win_t *)w;
    if (i >= 0 && i < o->ntex) {
        dmReplayPresent(CTX(o->tex[i]), o->crop);
        o->presents++;
    }
}
NVN(nvnWindowSetCrop, void, NVNwindow *w, int x, int y, int width, int height) {
    win_t *o = (win_t *)w;
    o->crop[0] = x; o->crop[1] = y; o->crop[2] = width; o->crop[3] = height;
}
NVN(nvnWindowGetCrop, void, const NVNwindow *w, NVNrectangle *out) {
    memcpy(out, ((const win_t *)w)->crop, sizeof(NVNrectangle));
}
NVN(nvnWindowSetPresentInterval, void, NVNwindow *w, int i) { ((win_t *)w)->interval = i; }
NVN(nvnWindowGetPresentInterval, int, const NVNwindow *w) { return ((const win_t *)w)->interval; }
NVN(nvnWindowFinalize, void, NVNwindow *w) { ((win_t *)w)->magic = 0; }
void dmNvnInitRuntime(void) {
    extern const runtime_ent __start_nvn_runtime[], __stop_nvn_runtime[];
    for (const runtime_ent *e = __start_nvn_runtime; e < __stop_nvn_runtime; e++)
        dmNvnRegister(e->name, e->fn);
}
