/*
 * dm_nvn.c — NVN 接口层(上游 dm/gfx/nvn/hle_nvn.c 的角色,HOS 最小集)。
 *
 * 游戏 main 只导入一个 NVN 入口 nvnBootstrapLoader,宿主 HLE_FORCE 强制
 * 接管后 SDK 里的 NVN 驱动一条指令都不执行。宿主按名字登记入口并返回
 * 静态 thunk(ARM64: dm_nvn_stubs.S 的 512 槽表;x86: 统一占位桩) ——
 * 每个入口第一次被调用时打一行 hilog(名字 + x0/x1/x2),这就是录制层
 * 的落点排期表。真实命令录制/回放(→ Vulkan)逐槽替换,当前呈现循环是
 * gfx/dm_vk 的清屏动画。
 */
#include "gfx/nvn/dm_nvn.h"
#include "host/dm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define NVN_TABLE_MAX 512

typedef struct {
    char name[64];
    uint8_t called;
} nvn_slot;

static nvn_slot g_table[NVN_TABLE_MAX];
static nvn_func_t g_fns[NVN_TABLE_MAX];
static uint32_t g_count;
static uint32_t g_called;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

/* Vulkan 呈现后端(dm_vk.cpp):把最近一次 NVN ClearColor 交给回放器画下一帧 */
extern void dmVkSetClearColor(float r, float g, float b, float a);
static float g_nvn_clear[4] = {0.10f, 0.04f, 0.03f, 1.0f};

/* 占位返回用的非空缓冲:让未实现的 NVN 入口返回非 0,避免引擎把空指针当对象/函数表用 */
uint8_t dm_nvn_dummy[0x100];

/* 未知 NVN 入口的统一占位函数:返回 0(NVN 句柄/结果),不跳空 */
static uint64_t nvn_unimpl_fn(void)
{
    return 0;
}

/* nvnDeviceGetProcAddress(device, name):游戏/引擎用它取 NVN 设备函数表。
 * 返回宿主已实现的真实函数;没有实现的回统一占位函数(非空,避免引擎把
 * 0 存进函数表后跳飞)。每个名字首次解析时打一行 hilog —— 这就是接下来
 * 要逐个实现的 NVN 录制入口清单。 */
/* 解析出的「宿主函数地址 -> NVN 名字」表:崩溃现场从游戏 GOT 槽读到函数
 * 指针后可以反查是哪个 NVN 入口(引擎在初始化时把两者写进自己的数据段)。 */
#define NVN_RESOLVED_MAX 640
static struct { uint64_t addr; char name[64]; } g_resolved[NVN_RESOLVED_MAX];
static int g_resolved_n;
static pthread_mutex_t g_resmu = PTHREAD_MUTEX_INITIALIZER;

static void nvn_record_resolved(uint64_t addr, const char *name)
{
    if (addr == 0 || name == NULL || name[0] == 0) {
        return;
    }
    pthread_mutex_lock(&g_resmu);
    for (int i = 0; i < g_resolved_n; i++) {
        if (g_resolved[i].addr == addr) {
            pthread_mutex_unlock(&g_resmu);
            return;
        }
    }
    if (g_resolved_n < NVN_RESOLVED_MAX) {
        g_resolved[g_resolved_n].addr = addr;
        snprintf(g_resolved[g_resolved_n].name, sizeof(g_resolved[0].name), "%s", name);
        g_resolved_n++;
    }
    pthread_mutex_unlock(&g_resmu);
}

const char *dmNvnNameForAddr(uint64_t addr)
{
    /* 崩溃处理器里调用:不加锁,只读已发布的表项 */
    const char *r = NULL;
    int n = g_resolved_n;
    for (int i = 0; i < n; i++) {
        if (g_resolved[i].addr == addr) {
            r = g_resolved[i].name;
            break;
        }
    }
    return r;
}

static uint64_t h_nvn_device_get_proc(void *dev, const char *name)
{
    (void)dev;
    static uint32_t n;
    if (name != NULL && name[0] != 0 && n < 128) {
        DMLOG_I("NVN proc: %{public}s", name);
        n++;
    }
    if (name != NULL) {
        pthread_mutex_lock(&g_mu);
        for (uint32_t i = 0; i < g_count; i++) {
            if (strcmp(g_table[i].name, name) == 0) {
                nvn_func_t fn = g_fns[i];
                pthread_mutex_unlock(&g_mu);
                if (fn != NULL) {
                    nvn_record_resolved((uint64_t)(uintptr_t)fn, name);
                    return (uint64_t)(uintptr_t)fn;
                }
                break;
            }
        }
        pthread_mutex_unlock(&g_mu);
    }
    nvn_record_resolved((uint64_t)(uintptr_t)&nvn_unimpl_fn, name);
    return (uint64_t)(uintptr_t)&nvn_unimpl_fn;
}

/* ================= NVN 对象模型(最小语义) =================
 * 游戏把 NVN 对象当不透明块分配(大小由 nvn.h 决定,几百字节),宿主只
 * 在块首写自己的小结构;游戏不会读内部字段,只把指针传回 API。
 * 先让 device/queue/window/commandBuffer/pool 生命周期走通、命令计数,
 * 下一阶段再把 op 流回放到 Vulkan。 */
#define NVN_MAGIC_DEV    0x5645444eU
#define NVN_MAGIC_QUEUE  0x4555514eU
#define NVN_MAGIC_WIN    0x4e49574eU
#define NVN_MAGIC_CB     0x42434d4eU
#define NVN_MAGIC_POOL   0x4c4f504eU

typedef struct { uint32_t magic; uint32_t flags; } nvn_devb_t;
typedef struct { uint32_t magic; uint32_t flags; } nvn_dev_t;
/* 必须 <= sizeof(NVNqueueBuilder)=64(见 nvn.h),否则会写坏相邻栈/堆内存 */
typedef struct { uint32_t magic; uint32_t flags; void *dev; uint64_t cmdsz, compsz, ctlsz; int32_t prio; uint32_t pad; } nvn_queueb_t;
typedef struct { uint32_t magic; void *dev; uint64_t submits; } nvn_queue_t;
typedef struct { uint32_t magic; void *dev; void *native; int32_t ntex, interval, flags; } nvn_winb_t;
typedef struct { uint32_t magic; void *dev; void *native; int32_t ntex, interval; uint64_t presents; } nvn_win_t;
typedef struct { uint32_t magic; void *dev; uint64_t size; void *mem; uint32_t flags; } nvn_poolb_t;
typedef struct { uint32_t magic; void *dev; uint64_t size; void *mem; } nvn_pool_t;
typedef struct {
    uint32_t magic; uint32_t recording;
    void *dev; const void *pool; int64_t pool_off; uint64_t pool_size;
    void *ctl; uint64_t ctl_size;
    uint64_t handles, ops, clears;
} nvn_cb_t;

static uint32_t g_nvn_devs, g_nvn_queues, g_nvn_wins, g_nvn_cbs, g_nvn_pools;
static uint64_t g_nvn_presents, g_nvn_submits, g_nvn_clears, g_nvn_ops;

/* ---- 真机实测:客体会读「NVN device 容器」base+0x448 处的一张函数表 ----
 * 设备对象在容器 base+0xA08;调用方对象在 base+0x360,它 +0xE8 正好是
 * base+0x448。表结构: [表+16]=条目数组;条目 24 字节, [条目+16]=函数指针。
 * 宿主先用一张全 stub 表填上,让引擎的这对分发调用不再跳空,再看下一步。 */
#define NVN_DISP_N 256
typedef struct { uint64_t a, b, fn; } nvn_disp_ent_t;   /* 24 bytes */
static nvn_disp_ent_t g_disp_ents[NVN_DISP_N];
static struct { uint64_t pad0, pad1; void *entries; } g_disp_table;

static uint64_t nvn_disp_stub(void)
{
    static uint8_t dummy[0x100];
    return (uint64_t)(uintptr_t)dummy;
}

static void nvn_fill_dispatch(void *dev)
{
    if (g_disp_table.entries == NULL) {
        for (int i = 0; i < NVN_DISP_N; i++) {
            g_disp_ents[i].a = 0;
            g_disp_ents[i].b = 0;
            g_disp_ents[i].fn = (uint64_t)(uintptr_t)&nvn_disp_stub;
        }
        g_disp_table.entries = g_disp_ents;
    }
    uint8_t *slot = (uint8_t *)dev - 0x5C0;   /* dev 在容器 +0xA08,表在 +0x448 */
    *(void **)slot = &g_disp_table;
    DMLOG_I("NVN device dispatch table filled at dev-0x5c0");
}

static void nvn_devb_defaults(void *b) { if (b) { memset(b, 0, sizeof(nvn_devb_t)); ((nvn_devb_t *)b)->magic = NVN_MAGIC_DEV; } }
static void nvn_devb_setflags(void *b, int f) { if (b) ((nvn_devb_t *)b)->flags = (uint32_t)f; }
static void nvn_devb_getflags(void *b, int *out) { if (b && out) *out = (int)((nvn_devb_t *)b)->flags; }
static uint32_t nvn_dev_init(void *d, const void *b)
{
    if (d == NULL) return 0;
    memset(d, 0, sizeof(nvn_dev_t));
    ((nvn_dev_t *)d)->magic = NVN_MAGIC_DEV;
    ((nvn_dev_t *)d)->flags = b ? ((const nvn_devb_t *)b)->flags : 0;
    DMLOG_I("NVN device init d=%{public}llx", (unsigned long long)(uintptr_t)d);
    g_nvn_devs++;
    return 1;
}
static void nvn_dev_finalize(void *d) { if (d) ((nvn_dev_t *)d)->magic = 0; }
static void nvn_dev_get_integer(const void *d, int info, int *out)
{
    (void)d; (void)info;
    /* 实验:引擎用它查询显存/尺寸类信息,0 可能导致后续 0 字节 arena */
    if (out) *out = 0;
}
static void nvn_dev_set_label(const void *d, const char *s) { (void)d; (void)s; }
static void nvn_dev_apply_deferred(void *d) { (void)d; }
static void nvn_dev_finalize_cmd(void *d, void *h) { (void)d; (void)h; }

static void nvn_qb_defaults(void *b)
{
    if (b) { memset(b, 0, sizeof(nvn_queueb_t)); ((nvn_queueb_t *)b)->magic = NVN_MAGIC_QUEUE; ((nvn_queueb_t *)b)->prio = 0; }
}
static void nvn_qb_setdev(void *b, void *d) { if (b) { ((nvn_queueb_t *)b)->dev = d; DMLOG_I("NVN qb setdev b=%{public}llx d=%{public}llx", (unsigned long long)(uintptr_t)b, (unsigned long long)(uintptr_t)d); } }
static void nvn_qb_setcmdsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->cmdsz = n; DMLOG_I("NVN qb cmdsz=%{public}llu", (unsigned long long)n); }
static void nvn_qb_setcompsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->compsz = n; }
static void nvn_qb_setctlsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->ctlsz = n; }
static void nvn_qb_setqmem(void *b, void *m, uint64_t n) { (void)b; DMLOG_I("NVN qb qmem=%{public}llx size=%{public}llu", (unsigned long long)(uintptr_t)m, (unsigned long long)n); }
static void nvn_qb_setflush(void *b, uint64_t n) { (void)b; (void)n; }
static void nvn_qb_setprio(void *b, int n) { if (b) ((nvn_queueb_t *)b)->prio = n; }
static uint32_t nvn_queue_init(void *q, const void *b)
{
    DMLOG_I("NVN queue init q=%{public}llx", (unsigned long long)(uintptr_t)q);
    if (q == NULL) return 0;
    memset(q, 0, sizeof(nvn_queue_t));
    ((nvn_queue_t *)q)->magic = NVN_MAGIC_QUEUE;
    ((nvn_queue_t *)q)->dev = b ? ((const nvn_queueb_t *)b)->dev : NULL;
    g_nvn_queues++;
    return 1;
}
static void nvn_queue_submit(void *q, int n, const void *handles)
{
    DMLOG_I("NVN queue submit q=%{public}llx n=%{public}d", (unsigned long long)(uintptr_t)q, n);
    (void)handles;
    if (q) ((nvn_queue_t *)q)->submits++;
    g_nvn_submits++;
    if (g_nvn_submits <= 3) {
        DMLOG_I("NVN submit #%{public}llu: %{public}d 个命令句柄", (unsigned long long)g_nvn_submits, n);
    }
}

static void nvn_winb_defaults(void *b)
{
    if (b) { memset(b, 0, sizeof(nvn_winb_t)); ((nvn_winb_t *)b)->magic = NVN_MAGIC_WIN; ((nvn_winb_t *)b)->interval = 1; ((nvn_winb_t *)b)->ntex = 2; }
}
static void nvn_winb_setdev(void *b, void *d) { if (b) ((nvn_winb_t *)b)->dev = d; }
static void nvn_winb_setnative(void *b, void *n) { if (b) ((nvn_winb_t *)b)->native = n; }
static void nvn_winb_setinterval(void *b, int n) { if (b) ((nvn_winb_t *)b)->interval = n; }
static void nvn_winb_setnbuf(void *b, int n) { if (b) ((nvn_winb_t *)b)->ntex = n; }
static uint32_t nvn_win_init(void *w, const void *b)
{
    DMLOG_I("NVN window init w=%{public}llx", (unsigned long long)(uintptr_t)w);
    if (w == NULL) return 0;
    memset(w, 0, sizeof(nvn_win_t));
    ((nvn_win_t *)w)->magic = NVN_MAGIC_WIN;
    if (b) {
        ((nvn_win_t *)w)->dev = ((const nvn_winb_t *)b)->dev;
        ((nvn_win_t *)w)->native = ((const nvn_winb_t *)b)->native;
        ((nvn_win_t *)w)->ntex = ((const nvn_winb_t *)b)->ntex;
        ((nvn_win_t *)w)->interval = ((const nvn_winb_t *)b)->interval;
    }
    g_nvn_wins++;
    DMLOG_I("NVN window 初始化: %{public}d 个缓冲, present interval=%{public}d",
            ((nvn_win_t *)w)->ntex, ((nvn_win_t *)w)->interval);
    return 1;
}
static void nvn_win_setactive(void *w, int n) { if (w) ((nvn_win_t *)w)->ntex = n; }
static uint32_t nvn_win_present(void *w, const void *tex)
{
    (void)tex;
    if (w) ((nvn_win_t *)w)->presents++;
    g_nvn_presents++;
    dmVkSetClearColor(g_nvn_clear[0], g_nvn_clear[1], g_nvn_clear[2], g_nvn_clear[3]);
    if (g_nvn_presents <= 5 || (g_nvn_presents % 300) == 0) {
        DMLOG_I("NVN present #%{public}llu", (unsigned long long)g_nvn_presents);
    }
    return 1;
}

static void nvn_poolb_defaults(void *b) { if (b) { memset(b, 0, sizeof(nvn_poolb_t)); ((nvn_poolb_t *)b)->magic = NVN_MAGIC_POOL; } }
static void nvn_poolb_setdev(void *b, void *d) { if (b) ((nvn_poolb_t *)b)->dev = d; }
static void nvn_poolb_setsize(void *b, uint64_t n) { if (b) ((nvn_poolb_t *)b)->size = n; DMLOG_I("NVN poolb size=%{public}llu", (unsigned long long)n); }
static void nvn_poolb_setmem(void *b, void *m, uint64_t n) { if (b) { ((nvn_poolb_t *)b)->mem = m; ((nvn_poolb_t *)b)->size = n; } DMLOG_I("NVN poolb storage mem=%{public}llx size=%{public}llu", (unsigned long long)(uintptr_t)m, (unsigned long long)n); }
static void nvn_poolb_setflags(void *b, int f) { if (b) ((nvn_poolb_t *)b)->flags = (uint32_t)f; }
static uint32_t nvn_pool_init(void *p, const void *b)
{
    DMLOG_I("NVN pool init p=%{public}llx", (unsigned long long)(uintptr_t)p);
    if (p == NULL) return 0;
    memset(p, 0, sizeof(nvn_pool_t));
    ((nvn_pool_t *)p)->magic = NVN_MAGIC_POOL;
    if (b) {
        ((nvn_pool_t *)p)->dev = ((const nvn_poolb_t *)b)->dev;
        ((nvn_pool_t *)p)->size = ((const nvn_poolb_t *)b)->size;
        ((nvn_pool_t *)p)->mem = ((const nvn_poolb_t *)b)->mem;
    }
    g_nvn_pools++;
    return 1;
}

static uint32_t nvn_cb_init(void *c, void *d)
{
    DMLOG_I("NVN cb init c=%{public}llx dev=%{public}llx", (unsigned long long)(uintptr_t)c, (unsigned long long)(uintptr_t)d);
    if (c == NULL) return 0;
    memset(c, 0, sizeof(nvn_cb_t));
    ((nvn_cb_t *)c)->magic = NVN_MAGIC_CB;
    ((nvn_cb_t *)c)->dev = d;
    g_nvn_cbs++;
    return 1;
}
static void nvn_cb_finalize(void *c) { if (c) ((nvn_cb_t *)c)->magic = 0; }
static void nvn_cb_setmemcb(void *c, void *fn, void *data) { (void)c; (void)fn; (void)data; }
static void nvn_cb_addcmdmem(void *c, const void *pool, int64_t off, uint64_t n)
{
    DMLOG_I("NVN cb addcmd pool=%{public}llx off=%{public}lld n=%{public}llu", (unsigned long long)(uintptr_t)pool, (long long)off, (unsigned long long)n);
    if (c) { ((nvn_cb_t *)c)->pool = pool; ((nvn_cb_t *)c)->pool_off = off; ((nvn_cb_t *)c)->pool_size = n; }
}
static void nvn_cb_addctlmem(void *c, void *m, uint64_t n)
{
    if (c) { ((nvn_cb_t *)c)->ctl = m; ((nvn_cb_t *)c)->ctl_size = n; }
}
static void nvn_cb_begin(void *c) { if (c) { ((nvn_cb_t *)c)->recording = 1; DMLOG_I("NVN cb begin c=%{public}llx", (unsigned long long)(uintptr_t)c); } }
static uint64_t nvn_cb_end(void *c)
{
    if (c == NULL) return 0;
    ((nvn_cb_t *)c)->recording = 0;
    return ++((nvn_cb_t *)c)->handles;
}
static uint32_t nvn_cb_isrec(const void *c) { return c ? ((const nvn_cb_t *)c)->recording : 0; }
static void nvn_cb_clear_color(void *c, int idx, const float *rgba, int mask)
{
    (void)idx; (void)mask;
    if (rgba != NULL) {
        g_nvn_clear[0] = rgba[0];
        g_nvn_clear[1] = rgba[1];
        g_nvn_clear[2] = rgba[2];
        g_nvn_clear[3] = rgba[3];
    }
    if (c) { ((nvn_cb_t *)c)->clears++; ((nvn_cb_t *)c)->ops++; }
    g_nvn_clears++; g_nvn_ops++;
    if (g_nvn_clears <= 3) DMLOG_I("NVN ClearColor idx=%{public}d mask=%{public}#x", idx, mask);
}
static void nvn_cb_clear_ds(void *c, float d, uint32_t dm, int s, uint32_t sm)
{
    (void)d; (void)dm; (void)s; (void)sm;
    if (c) { ((nvn_cb_t *)c)->ops++; }
    g_nvn_ops++;
}
static void nvn_cb_setrt(void *c, int n, const void *const *t, const void *const *v, const void *depth)
{
    (void)n; (void)t; (void)v; (void)depth;
    if (c) ((nvn_cb_t *)c)->ops++;
    g_nvn_ops++;
}
static void nvn_cb_viewports(void *c, int first, int n, const float *v) { (void)first; (void)n; (void)v; if (c) ((nvn_cb_t *)c)->ops++; g_nvn_ops++; }
static void nvn_cb_scissor(void *c, int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; if (c) ((nvn_cb_t *)c)->ops++; g_nvn_ops++; }
static void nvn_cb_depthrange(void *c, float n, float f) { (void)n; (void)f; if (c) ((nvn_cb_t *)c)->ops++; g_nvn_ops++; }

#include "dm_nvn_trace.inc"

/* ---- 对象构造类:游戏把「初始化是否成功」当布尔用,必须回 1 ---- */
typedef struct { uint32_t magic; void *dev; } nvn_generic_t;

static uint32_t nvn_generic_init(void *o, void *dev, uint32_t magic)
{
    if (o == NULL) return 0;
    memset(o, 0, sizeof(nvn_generic_t));
    ((nvn_generic_t *)o)->magic = magic;
    ((nvn_generic_t *)o)->dev = dev;
    return 1;
}
static uint32_t nvn_program_init(void *p, void *d) { return nvn_generic_init(p, d, 0x474f5250U); }   /* "PROG" */
static void nvn_program_set_shaders(void *p, int n, const void *s) { (void)p; (void)n; (void)s; }
static void nvn_texture_pool_init(void *o, void *pool, int64_t off, uint64_t n)
{
    (void)pool; (void)off; (void)n;
    if (o) { memset(o, 0, sizeof(nvn_generic_t)); ((nvn_generic_t *)o)->magic = 0x58545450U; }
}
static void nvn_sampler_pool_init(void *o, void *pool, int64_t off, uint64_t n)
{
    (void)pool; (void)off; (void)n;
    if (o) { memset(o, 0, sizeof(nvn_generic_t)); ((nvn_generic_t *)o)->magic = 0x504d4153U; }
}
static uint32_t nvn_texture_init(void *t, void *d) { return nvn_generic_init(t, d, 0x58544554U); }
static uint32_t nvn_sampler_init(void *s, void *d) { return nvn_generic_init(s, d, 0x504d4153U); }
static uint32_t nvn_buffer_init(void *b, void *d, uint64_t flags) { (void)flags; return nvn_generic_init(b, d, 0x4655424eU); }

/* ---- 内存池查询 ---- */
/* nvnMemoryPoolGetBufferAddress(const NVNmemoryPool*) -> NVNbufferAddress
 * (没有 offset 参数!之前按 (pool, off) 实现,把引擎传的指针当成偏移,
 * 返回了越界地址;真机日志里 off=409355664032 就是铁证。) */
static uint64_t nvn_pool_addr(const void *pool)
{
    const nvn_pool_t *p = (const nvn_pool_t *)pool;
    if (p == NULL) return 0;
    return (uint64_t)(uintptr_t)p->mem;
}
static uint64_t nvn_pool_map(void *pool) { const nvn_pool_t *p = pool; return p ? (uint64_t)(uintptr_t)p->mem : 0; }
/* nvnMemoryPoolMapVirtual(pool, count, const NVNmappingRequest*) -> NVNboolean */
static uint32_t nvn_pool_map_virtual(void *pool, int count, const void *req)
{
    (void)pool; (void)count; (void)req;
    return 1;
}
static uint64_t nvn_pool_get_size(const void *pool) { const nvn_pool_t *p = pool; return p ? p->size : 0; }
static uint32_t nvn_pool_get_flags(const void *pool) { (void)pool; return 0; }   /* NVNmemoryPoolFlags(枚举,直接返回) */
static uint64_t nvn_poolb_get_mem(const void *b) { const nvn_poolb_t *q = b; return q ? (uint64_t)(uintptr_t)q->mem : 0; }
static uint64_t nvn_poolb_get_size(const void *b) { const nvn_poolb_t *q = b; return q ? q->size : 0; }
static uint32_t nvn_poolb_get_flags(const void *b) { const nvn_poolb_t *q = b; return q ? q->flags : 0; }
static void nvn_pool_flush(void *pool, int64_t off, uint64_t n) { (void)pool; (void)off; (void)n; }
static void nvn_pool_invalidate(void *pool, int64_t off, uint64_t n) { (void)pool; (void)off; (void)n; }
static void nvn_pool_finalize(void *pool) { if (pool) ((nvn_pool_t *)pool)->magic = 0; }

/* ---- 队列/窗口 builder 的查询 ---- */
static uint64_t nvn_qb_get_qmemsz(const void *b)
{
    DMLOG_I("NVN qb get qmemsz b=%{public}llx", (unsigned long long)(uintptr_t)b);
    const nvn_queueb_t *q = b;
    if (q == NULL) return 0;
    return (q->cmdsz + q->compsz + q->ctlsz + 4095) & ~(uint64_t)4095;
}
static void *nvn_qb_get_dev(const void *b) { const nvn_queueb_t *q = b; return q ? q->dev : NULL; }
static void nvn_qb_get_flags(const void *b, int *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->flags; }
static void nvn_qb_get_cmdsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->cmdsz; }
static void nvn_qb_get_compsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->compsz; }
static void nvn_qb_get_ctlsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->ctlsz; }
static void nvn_qb_get_flush(const void *b, uint64_t *out) { (void)b; if (out) *out = 0; }
static uint64_t nvn_qb_get_memsz(const void *b) { return nvn_qb_get_qmemsz(b); }
static void nvn_qb_get_mem(const void *b, uint64_t *out) { (void)b; if (out) *out = 0; }

/* ---- 队列收尾/present ---- */
static void nvn_queue_flush(void *q) { (void)q; }
static void nvn_queue_finish(void *q) { (void)q; }
static uint32_t nvn_queue_present(void *q, const void *tex) { (void)q; (void)tex; g_nvn_presents++; if (g_nvn_presents <= 5) DMLOG_I("NVN QueuePresentTexture #%{public}llu", (unsigned long long)g_nvn_presents); return 1; }
static uint32_t nvn_queue_acquire(void *q, const void *tex) { (void)q; (void)tex; return 1; }
static uint32_t nvn_window_acquire(void *w, const void *tex) { (void)w; (void)tex; return 1; }
static void nvn_window_finalize(void *w) { if (w) ((nvn_win_t *)w)->magic = 0; }
static void nvn_window_set_interval(void *w, int n) { if (w) ((nvn_win_t *)w)->interval = n; }
/* nvnWindowGetPresentInterval(window) -> int, nvnWindowGetNativeWindow(window) -> NVNnativeWindow */
static int nvn_window_get_interval(const void *w) { return w ? ((const nvn_win_t *)w)->interval : 0; }
static void *nvn_window_get_native(const void *w) { return w ? ((const nvn_win_t *)w)->native : NULL; }
/* window builder 的对应 getter */
static int nvn_winb_get_interval(const void *b) { return b ? ((const nvn_winb_t *)b)->interval : 0; }
static void *nvn_winb_get_native(const void *b) { return b ? ((const nvn_winb_t *)b)->native : NULL; }
static int nvn_winb_get_nbuf(const void *b) { return b ? ((const nvn_winb_t *)b)->ntex : 0; }

/* ---- 纹理/缓冲 builder:nvn.h 里 GetStorageSize/Alignment/GetFlags 都是
 * 「直接返回」而不是写 out 参数。之前用跟踪桩返回 0,引擎按 0 字节/0 对齐
 * 分配存储,随后分配器返回 NULL → 崩溃(真机日志:640x384 纹理构建后崩)。 */
#define NVN_MAGIC_TEXB 0x42584554U
#define NVN_MAGIC_BUFB 0x42465542U
typedef struct { uint32_t magic, flags, target, format; int32_t width, height, depth, levels, samples; } nvn_texb_t;
typedef struct { uint32_t magic; void *dev; void *pool; int64_t off; uint64_t size; } nvn_bufb_t;

static void nvn_texb_defaults(void *b)
{
    if (b) { memset(b, 0, sizeof(nvn_texb_t)); ((nvn_texb_t *)b)->magic = NVN_MAGIC_TEXB; ((nvn_texb_t *)b)->width = 1; ((nvn_texb_t *)b)->height = 1; ((nvn_texb_t *)b)->depth = 1; ((nvn_texb_t *)b)->levels = 1; }
}
static void nvn_texb_setflags(void *b, int f) { if (b) ((nvn_texb_t *)b)->flags = (uint32_t)f; }
static uint32_t nvn_texb_getflags(const void *b) { return b ? ((const nvn_texb_t *)b)->flags : 0; }
static void nvn_texb_setdev(void *b, void *d) { (void)b; (void)d; }
static void nvn_texb_settarget(void *b, int t) { if (b) ((nvn_texb_t *)b)->target = (uint32_t)t; }
static void nvn_texb_setwidth(void *b, int w) { if (b) ((nvn_texb_t *)b)->width = w; }
static void nvn_texb_setheight(void *b, int h) { if (b) ((nvn_texb_t *)b)->height = h; }
static void nvn_texb_setdepth(void *b, int d) { if (b) ((nvn_texb_t *)b)->depth = d; }
static void nvn_texb_setlevels(void *b, int l) { if (b) ((nvn_texb_t *)b)->levels = l; }
static void nvn_texb_setformat(void *b, int f) { if (b) ((nvn_texb_t *)b)->format = (uint32_t)f; }
static void nvn_texb_setsamples(void *b, int s) { if (b) ((nvn_texb_t *)b)->samples = s; }
static uint64_t nvn_texb_size(const void *b)
{
    const nvn_texb_t *t = (const nvn_texb_t *)b;
    if (t == NULL) return 0;
    uint64_t px = (uint64_t)(t->width > 0 ? t->width : 1) * (uint64_t)(t->height > 0 ? t->height : 1);
    uint64_t depth = t->depth > 0 ? (uint64_t)t->depth : 1;
    uint64_t levels = t->levels > 0 ? (uint64_t)t->levels : 1;
    uint64_t bytes = px * depth * levels * 4;    /* 保守按 4 字节/像素 */
    return (bytes + 4095) & ~(uint64_t)4095;
}
static uint64_t nvn_texb_align(const void *b) { (void)b; return 4096; }

static void nvn_bufb_defaults(void *b) { if (b) { memset(b, 0, sizeof(nvn_bufb_t)); ((nvn_bufb_t *)b)->magic = NVN_MAGIC_BUFB; } }
static void nvn_bufb_setdev(void *b, void *d) { if (b) ((nvn_bufb_t *)b)->dev = d; }
static void nvn_bufb_setsize(void *b, uint64_t n) { if (b) ((nvn_bufb_t *)b)->size = n; }
static void nvn_bufb_setstorage(void *b, void *pool, int64_t off, uint64_t n)
{
    if (b) { ((nvn_bufb_t *)b)->pool = pool; ((nvn_bufb_t *)b)->off = off; ((nvn_bufb_t *)b)->size = n; }
}
static uint64_t nvn_bufb_getsize(const void *b) { return b ? ((const nvn_bufb_t *)b)->size : 0; }
static void *nvn_bufb_getpool(const void *b) { return b ? ((const nvn_bufb_t *)b)->pool : NULL; }
static int64_t nvn_bufb_getoff(const void *b) { return b ? ((const nvn_bufb_t *)b)->off : 0; }

void dmNvnInit(void)
{
    dm_nvn_register_traces();
    dmNvnRegister("nvnDeviceGetProcAddress", (nvn_func_t)h_nvn_device_get_proc);
    dmNvnRegister("nvnDeviceBuilderSetDefaults", (nvn_func_t)nvn_devb_defaults);
    dmNvnRegister("nvnDeviceBuilderSetFlags", (nvn_func_t)nvn_devb_setflags);
    dmNvnRegister("nvnDeviceBuilderGetFlags", (nvn_func_t)nvn_devb_getflags);
    dmNvnRegister("nvnDeviceInitialize", (nvn_func_t)nvn_dev_init);
    dmNvnRegister("nvnDeviceFinalize", (nvn_func_t)nvn_dev_finalize);
    dmNvnRegister("nvnDeviceGetInteger", (nvn_func_t)nvn_dev_get_integer);
    dmNvnRegister("nvnDeviceSetDebugLabel", (nvn_func_t)nvn_dev_set_label);
    dmNvnRegister("nvnDeviceApplyDeferredFinalizes", (nvn_func_t)nvn_dev_apply_deferred);
    dmNvnRegister("nvnDeviceFinalizeCommandHandle", (nvn_func_t)nvn_dev_finalize_cmd);

    dmNvnRegister("nvnQueueBuilderSetDefaults", (nvn_func_t)nvn_qb_defaults);
    dmNvnRegister("nvnQueueBuilderSetDevice", (nvn_func_t)nvn_qb_setdev);
    dmNvnRegister("nvnQueueBuilderSetCommandMemorySize", (nvn_func_t)nvn_qb_setcmdsz);
    dmNvnRegister("nvnQueueBuilderSetComputeMemorySize", (nvn_func_t)nvn_qb_setcompsz);
    dmNvnRegister("nvnQueueBuilderSetControlMemorySize", (nvn_func_t)nvn_qb_setctlsz);
    dmNvnRegister("nvnQueueBuilderSetQueueMemory", (nvn_func_t)nvn_qb_setqmem);
    dmNvnRegister("nvnQueueBuilderSetCommandFlushThreshold", (nvn_func_t)nvn_qb_setflush);
    dmNvnRegister("nvnQueueBuilderSetQueuePriority", (nvn_func_t)nvn_qb_setprio);
    dmNvnRegister("nvnQueueInitialize", (nvn_func_t)nvn_queue_init);
    dmNvnRegister("nvnQueueSubmitCommands", (nvn_func_t)nvn_queue_submit);

    dmNvnRegister("nvnWindowBuilderSetDefaults", (nvn_func_t)nvn_winb_defaults);
    dmNvnRegister("nvnWindowBuilderSetDevice", (nvn_func_t)nvn_winb_setdev);
    dmNvnRegister("nvnWindowBuilderSetNativeWindow", (nvn_func_t)nvn_winb_setnative);
    dmNvnRegister("nvnWindowBuilderSetPresentInterval", (nvn_func_t)nvn_winb_setinterval);
    dmNvnRegister("nvnWindowBuilderSetNumBuffers", (nvn_func_t)nvn_winb_setnbuf);
    dmNvnRegister("nvnWindowInitialize", (nvn_func_t)nvn_win_init);
    dmNvnRegister("nvnWindowSetNumActiveTextures", (nvn_func_t)nvn_win_setactive);
    dmNvnRegister("nvnWindowPresent", (nvn_func_t)nvn_win_present);

    dmNvnRegister("nvnMemoryPoolBuilderSetDefaults", (nvn_func_t)nvn_poolb_defaults);
    dmNvnRegister("nvnMemoryPoolBuilderSetDevice", (nvn_func_t)nvn_poolb_setdev);
    dmNvnRegister("nvnMemoryPoolBuilderSetSize", (nvn_func_t)nvn_poolb_setsize);
    dmNvnRegister("nvnMemoryPoolBuilderSetStorage", (nvn_func_t)nvn_poolb_setmem);
    dmNvnRegister("nvnMemoryPoolBuilderSetFlags", (nvn_func_t)nvn_poolb_setflags);
    dmNvnRegister("nvnMemoryPoolBuilderGetMemory", (nvn_func_t)nvn_poolb_get_mem);
    dmNvnRegister("nvnMemoryPoolBuilderGetSize", (nvn_func_t)nvn_poolb_get_size);
    dmNvnRegister("nvnMemoryPoolBuilderGetFlags", (nvn_func_t)nvn_poolb_get_flags);
    dmNvnRegister("nvnMemoryPoolInitialize", (nvn_func_t)nvn_pool_init);

    dmNvnRegister("nvnCommandBufferInitialize", (nvn_func_t)nvn_cb_init);
    dmNvnRegister("nvnCommandBufferFinalize", (nvn_func_t)nvn_cb_finalize);
    dmNvnRegister("nvnCommandBufferSetMemoryCallback", (nvn_func_t)nvn_cb_setmemcb);
    dmNvnRegister("nvnCommandBufferSetMemoryCallbackData", (nvn_func_t)nvn_cb_setmemcb);
    dmNvnRegister("nvnCommandBufferAddCommandMemory", (nvn_func_t)nvn_cb_addcmdmem);
    dmNvnRegister("nvnCommandBufferAddControlMemory", (nvn_func_t)nvn_cb_addctlmem);
    dmNvnRegister("nvnCommandBufferBeginRecording", (nvn_func_t)nvn_cb_begin);
    dmNvnRegister("nvnCommandBufferEndRecording", (nvn_func_t)nvn_cb_end);
    dmNvnRegister("nvnCommandBufferIsRecording", (nvn_func_t)nvn_cb_isrec);
    dmNvnRegister("nvnCommandBufferClearColor", (nvn_func_t)nvn_cb_clear_color);
    dmNvnRegister("nvnCommandBufferClearDepthStencil", (nvn_func_t)nvn_cb_clear_ds);
    dmNvnRegister("nvnCommandBufferSetRenderTargets", (nvn_func_t)nvn_cb_setrt);
    dmNvnRegister("nvnCommandBufferSetViewports", (nvn_func_t)nvn_cb_viewports);
    dmNvnRegister("nvnCommandBufferSetScissor", (nvn_func_t)nvn_cb_scissor);
    dmNvnRegister("nvnCommandBufferSetDepthRange", (nvn_func_t)nvn_cb_depthrange);

    /* 对象构造/查询:必须返回 1 或有效值,不能落到 0 的跟踪桩 */
    dmNvnRegister("nvnProgramInitialize", (nvn_func_t)nvn_program_init);
    dmNvnRegister("nvnProgramSetShaders", (nvn_func_t)nvn_program_set_shaders);
    dmNvnRegister("nvnProgramSetShadersExt", (nvn_func_t)nvn_program_set_shaders);
    dmNvnRegister("nvnTexturePoolInitialize", (nvn_func_t)nvn_texture_pool_init);
    dmNvnRegister("nvnSamplerPoolInitialize", (nvn_func_t)nvn_sampler_pool_init);
    dmNvnRegister("nvnTextureInitialize", (nvn_func_t)nvn_texture_init);
    dmNvnRegister("nvnSamplerInitialize", (nvn_func_t)nvn_sampler_init);
    dmNvnRegister("nvnBufferInitialize", (nvn_func_t)nvn_buffer_init);
    dmNvnRegister("nvnMemoryPoolGetBufferAddress", (nvn_func_t)nvn_pool_addr);
    dmNvnRegister("nvnMemoryPoolMap", (nvn_func_t)nvn_pool_map);
    dmNvnRegister("nvnMemoryPoolMapVirtual", (nvn_func_t)nvn_pool_map_virtual);
    dmNvnRegister("nvnMemoryPoolGetSize", (nvn_func_t)nvn_pool_get_size);
    dmNvnRegister("nvnMemoryPoolGetFlags", (nvn_func_t)nvn_pool_get_flags);
    dmNvnRegister("nvnMemoryPoolFlushMappedRange", (nvn_func_t)nvn_pool_flush);
    dmNvnRegister("nvnMemoryPoolInvalidateMappedRange", (nvn_func_t)nvn_pool_invalidate);
    dmNvnRegister("nvnMemoryPoolFinalize", (nvn_func_t)nvn_pool_finalize);
    dmNvnRegister("nvnQueueBuilderGetQueueMemorySize", (nvn_func_t)nvn_qb_get_qmemsz);
    dmNvnRegister("nvnQueueBuilderGetDevice", (nvn_func_t)nvn_qb_get_dev);
    dmNvnRegister("nvnQueueBuilderGetFlags", (nvn_func_t)nvn_qb_get_flags);
    dmNvnRegister("nvnQueueBuilderGetCommandMemorySize", (nvn_func_t)nvn_qb_get_cmdsz);
    dmNvnRegister("nvnQueueBuilderGetComputeMemorySize", (nvn_func_t)nvn_qb_get_compsz);
    dmNvnRegister("nvnQueueBuilderGetControlMemorySize", (nvn_func_t)nvn_qb_get_ctlsz);
    dmNvnRegister("nvnQueueBuilderGetCommandFlushThreshold", (nvn_func_t)nvn_qb_get_flush);
    dmNvnRegister("nvnQueueBuilderGetMemorySize", (nvn_func_t)nvn_qb_get_memsz);
    dmNvnRegister("nvnQueueBuilderGetMemory", (nvn_func_t)nvn_qb_get_mem);
    dmNvnRegister("nvnQueueFlush", (nvn_func_t)nvn_queue_flush);
    dmNvnRegister("nvnQueueFinish", (nvn_func_t)nvn_queue_finish);
    dmNvnRegister("nvnQueuePresentTexture", (nvn_func_t)nvn_queue_present);
    dmNvnRegister("nvnQueueAcquireTexture", (nvn_func_t)nvn_queue_acquire);
    dmNvnRegister("nvnWindowAcquireTexture", (nvn_func_t)nvn_window_acquire);
    dmNvnRegister("nvnWindowFinalize", (nvn_func_t)nvn_window_finalize);
    dmNvnRegister("nvnWindowSetPresentInterval", (nvn_func_t)nvn_window_set_interval);
    dmNvnRegister("nvnWindowGetPresentInterval", (nvn_func_t)nvn_window_get_interval);
    dmNvnRegister("nvnWindowGetNativeWindow", (nvn_func_t)nvn_window_get_native);
    dmNvnRegister("nvnWindowBuilderGetNativeWindow", (nvn_func_t)nvn_winb_get_native);
    dmNvnRegister("nvnWindowBuilderGetPresentInterval", (nvn_func_t)nvn_winb_get_interval);
    dmNvnRegister("nvnWindowBuilderGetNumBuffers", (nvn_func_t)nvn_winb_get_nbuf);

    /* 纹理/缓冲 builder(Get* 直接返回值;存储尺寸不能再是 0) */
    dmNvnRegister("nvnTextureBuilderSetDefaults", (nvn_func_t)nvn_texb_defaults);
    dmNvnRegister("nvnTextureBuilderSetDevice", (nvn_func_t)nvn_texb_setdev);
    dmNvnRegister("nvnTextureBuilderSetFlags", (nvn_func_t)nvn_texb_setflags);
    dmNvnRegister("nvnTextureBuilderGetFlags", (nvn_func_t)nvn_texb_getflags);
    dmNvnRegister("nvnTextureBuilderSetTarget", (nvn_func_t)nvn_texb_settarget);
    dmNvnRegister("nvnTextureBuilderSetWidth", (nvn_func_t)nvn_texb_setwidth);
    dmNvnRegister("nvnTextureBuilderSetHeight", (nvn_func_t)nvn_texb_setheight);
    dmNvnRegister("nvnTextureBuilderSetDepth", (nvn_func_t)nvn_texb_setdepth);
    dmNvnRegister("nvnTextureBuilderSetLevels", (nvn_func_t)nvn_texb_setlevels);
    dmNvnRegister("nvnTextureBuilderSetFormat", (nvn_func_t)nvn_texb_setformat);
    dmNvnRegister("nvnTextureBuilderSetSamples", (nvn_func_t)nvn_texb_setsamples);
    dmNvnRegister("nvnTextureBuilderGetStorageSize", (nvn_func_t)nvn_texb_size);
    dmNvnRegister("nvnTextureBuilderGetStorageAlignment", (nvn_func_t)nvn_texb_align);
    dmNvnRegister("nvnBufferBuilderSetDefaults", (nvn_func_t)nvn_bufb_defaults);
    dmNvnRegister("nvnBufferBuilderSetDevice", (nvn_func_t)nvn_bufb_setdev);
    dmNvnRegister("nvnBufferBuilderSetSize", (nvn_func_t)nvn_bufb_setsize);
    dmNvnRegister("nvnBufferBuilderSetStorage", (nvn_func_t)nvn_bufb_setstorage);
    dmNvnRegister("nvnBufferBuilderGetSize", (nvn_func_t)nvn_bufb_getsize);
    dmNvnRegister("nvnBufferBuilderGetMemoryPool", (nvn_func_t)nvn_bufb_getpool);
    dmNvnRegister("nvnBufferBuilderGetMemoryOffset", (nvn_func_t)nvn_bufb_getoff);
}

#ifdef __aarch64__
extern uint8_t dm_nvn_thunks[];

uint64_t dm_nvn_unimpl_c(uint64_t idx, uint64_t *regs)
{
    pthread_mutex_lock(&g_mu);
    if (idx < g_count && !g_table[idx].called) {
        g_table[idx].called = 1;
        g_called++;
        DMLOG_W("NVN %{public}s: first call (thunk, args x0=%{public}llx x1=%{public}llx x2=%{public}llx)",
                g_table[idx].name, (unsigned long long)regs[0],
                (unsigned long long)regs[1], (unsigned long long)regs[2]);
    }
    pthread_mutex_unlock(&g_mu);
    return 0;
}
#else
static uint64_t nvn_stub0(void)
{
    return 0;
}
#endif

void dmNvnRegister(const char *name, nvn_func_t fn)
{
    if (name == NULL) {
        return;
    }
    pthread_mutex_lock(&g_mu);
    for (uint32_t i = 0; i < g_count; i++) {
        if (strcmp(g_table[i].name, name) == 0) {
            g_fns[i] = fn;
            pthread_mutex_unlock(&g_mu);
            return;
        }
    }
    if (g_count < NVN_TABLE_MAX) {
        snprintf(g_table[g_count].name, sizeof(g_table[0].name), "%s", name);
        g_table[g_count].called = 0;
        g_fns[g_count] = fn;
        g_count++;
    }
    pthread_mutex_unlock(&g_mu);
}

uint64_t dmNvnBootstrapLoader(const char *name)
{
    if (name == NULL || name[0] == 0) {
        return 0;
    }
    uint32_t id;
    pthread_mutex_lock(&g_mu);
    for (id = 0; id < g_count; id++) {
        if (strcmp(g_table[id].name, name) == 0) {
            break;
        }
    }
    if (id == g_count) {
        if (g_count >= NVN_TABLE_MAX) {
            pthread_mutex_unlock(&g_mu);
            DMLOG_E("NVN %{public}s: 入口表满", name);
            return 0;
        }
        snprintf(g_table[g_count].name, sizeof(g_table[0].name), "%s", name);
        g_table[g_count].called = 0;
        g_count++;
    }
    pthread_mutex_unlock(&g_mu);
    if (id < g_count && g_fns[id] != NULL) {
        nvn_record_resolved((uint64_t)(uintptr_t)g_fns[id], name);
        return (uint64_t)(uintptr_t)g_fns[id];   /* 已实现/录制入口 */
    }
#ifdef __aarch64__
    {
        uint64_t r = (uint64_t)(uintptr_t)(dm_nvn_thunks + (size_t)id * 8);
        nvn_record_resolved(r, name);
        return r;
    }
#else
    return (uint64_t)(uintptr_t)&nvn_stub0;   /* data-only:表/名字自检,不执行 */
#endif
}

/* 上游同一接管点:main 导入、sdk 定义,必须 force */
HLE_FORCE("nvnBootstrapLoader", "NVN 图形接口整块换成宿主(后续 Vulkan 回放),sdk 的 NVN 驱动不执行",
          void *, h_nvn_bootstrap, const char *name)
{
    static uint32_t boot_n;
    uint64_t r = dmNvnBootstrapLoader(name);
    if (boot_n < 48) {
        DMLOG_I("NVN bootstrap(%{public}s) -> %{public}llx", name ? name : "?", (unsigned long long)r);
        boot_n++;
    }
    return (void *)(uintptr_t)r;
}

/* SDK 的 NVN 驱动初始化入口:正常路径由宿主 nvnBootstrapLoader 拦截,不应执行。
 * 真机实测仍会走到 nv::InitializeGraphics → NvOsDrvOpen → sm: IPC,
 * 这里再按符号强制接管一次(重定位能覆盖到的调用点),把驱动整段挡掉。 */
HLE_FORCE("_ZN2nv18InitializeGraphicsEPvm", "NVN 驱动初始化:宿主不跑 sdk 图形驱动", uint32_t,
          h_nv_InitGfx, void *mem, size_t size) { (void)mem; (void)size; return 0; }
HLE_FORCE("_ZN2nv16FinalizeGraphicsEv", "NVN 驱动收尾:空操作", void, h_nv_FinGfx, void) {}
HLE_FORCE("_ZN2nv22SetGraphicsServiceNameEPKc", "NVN 服务名:记录但不连服务", void,
          h_nv_SetName, const char *n) { (void)n; }
HLE_FORCE("_ZN2nv26InitializeGraphicsDevtoolsEPvm", "NVN devtools:宿主不支持", uint32_t,
          h_nv_InitDev, void *mem, size_t size) { (void)mem; (void)size; return 0; }
HLE_FORCE("_ZN2nv21GetGraphicsMemoryInfoEPNS_10MemoryInfoE", "NVN 显存统计:给 0", void,
          h_nv_MemInfo, void *info) { (void)info; }

uint32_t dmNvnEntryCount(void)
{
    return g_count;
}

uint32_t dmNvnCalledCount(void)
{
    return g_called;
}

const char *dmNvnSelfTest(void)
{
    static char report[192];
    /* 与上游一致的引导序列:先取构建器/设备/呈现入口 */
    dmNvnBootstrapLoader("nvnDeviceBuilderInitialize");
    dmNvnBootstrapLoader("nvnDeviceInitialize");
    dmNvnBootstrapLoader("nvnQueuePresent");
    uint64_t a = dmNvnBootstrapLoader("nvnDeviceInitialize");
    uint64_t b = dmNvnBootstrapLoader("nvnNoSuchEntry");
    snprintf(report, sizeof(report),
             "nvn: table=%u called=%u dispatch=%s unknown->%s",
             dmNvnEntryCount(), dmNvnCalledCount(),
             a ? "ok" : "null", b == 0 ? "null" : "non-null");
    return report;
}
















