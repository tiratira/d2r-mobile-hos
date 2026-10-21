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

/* 未知 NVN 入口的统一占位函数:返回 0(NVN 句柄/结果),不跳空 */
static uint64_t nvn_unimpl_fn(void)
{
    return 0;
}

/* nvnDeviceGetProcAddress(device, name):游戏/引擎用它取 NVN 设备函数表。
 * 返回宿主已实现的真实函数;没有实现的回统一占位函数(非空,避免引擎把
 * 0 存进函数表后跳飞)。每个名字首次解析时打一行 hilog —— 这就是接下来
 * 要逐个实现的 NVN 录制入口清单。 */
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
                    return (uint64_t)(uintptr_t)fn;
                }
                break;
            }
        }
        pthread_mutex_unlock(&g_mu);
    }
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
typedef struct { uint32_t magic; void *dev; uint64_t cmdsz, compsz, ctlsz, flush, qmem, qmemsz; int32_t prio, flags; } nvn_queueb_t;
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

static void nvn_devb_defaults(void *b) { if (b) { memset(b, 0, sizeof(nvn_devb_t)); ((nvn_devb_t *)b)->magic = NVN_MAGIC_DEV; } }
static void nvn_devb_setflags(void *b, int f) { if (b) ((nvn_devb_t *)b)->flags = (uint32_t)f; }
static void nvn_devb_getflags(void *b, int *out) { if (b && out) *out = (int)((nvn_devb_t *)b)->flags; }
static uint32_t nvn_dev_init(void *d, const void *b)
{
    if (d == NULL) return 0;
    memset(d, 0, sizeof(nvn_dev_t));
    ((nvn_dev_t *)d)->magic = NVN_MAGIC_DEV;
    ((nvn_dev_t *)d)->flags = b ? ((const nvn_devb_t *)b)->flags : 0;
    DMLOG_I("NVN device 初始化(宿主对象模型)");
    g_nvn_devs++;
    return 1;
}
static void nvn_dev_finalize(void *d) { if (d) ((nvn_dev_t *)d)->magic = 0; }
static void nvn_dev_get_integer(const void *d, int info, int *out)
{
    (void)d; (void)info;
    if (out) *out = 0;
}
static void nvn_dev_set_label(const void *d, const char *s) { (void)d; (void)s; }
static void nvn_dev_apply_deferred(void *d) { (void)d; }
static void nvn_dev_finalize_cmd(void *d, void *h) { (void)d; (void)h; }

static void nvn_qb_defaults(void *b)
{
    if (b) { memset(b, 0, sizeof(nvn_queueb_t)); ((nvn_queueb_t *)b)->magic = NVN_MAGIC_QUEUE; ((nvn_queueb_t *)b)->prio = 0; }
}
static void nvn_qb_setdev(void *b, void *d) { if (b) ((nvn_queueb_t *)b)->dev = d; }
static void nvn_qb_setcmdsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->cmdsz = n; }
static void nvn_qb_setcompsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->compsz = n; }
static void nvn_qb_setctlsz(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->ctlsz = n; }
static void nvn_qb_setqmem(void *b, void *m, uint64_t n) { if (b) { ((nvn_queueb_t *)b)->qmem = (uint64_t)(uintptr_t)m; ((nvn_queueb_t *)b)->qmemsz = n; } }
static void nvn_qb_setflush(void *b, uint64_t n) { if (b) ((nvn_queueb_t *)b)->flush = n; }
static void nvn_qb_setprio(void *b, int n) { if (b) ((nvn_queueb_t *)b)->prio = n; }
static uint32_t nvn_queue_init(void *q, const void *b)
{
    if (q == NULL) return 0;
    memset(q, 0, sizeof(nvn_queue_t));
    ((nvn_queue_t *)q)->magic = NVN_MAGIC_QUEUE;
    ((nvn_queue_t *)q)->dev = b ? ((const nvn_queueb_t *)b)->dev : NULL;
    g_nvn_queues++;
    return 1;
}
static void nvn_queue_submit(void *q, int n, const void *handles)
{
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
static void nvn_poolb_setsize(void *b, uint64_t n) { if (b) ((nvn_poolb_t *)b)->size = n; }
static void nvn_poolb_setmem(void *b, void *m) { if (b) ((nvn_poolb_t *)b)->mem = m; }
static void nvn_poolb_setflags(void *b, int f) { if (b) ((nvn_poolb_t *)b)->flags = (uint32_t)f; }
static uint32_t nvn_pool_init(void *p, const void *b)
{
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
    if (c) { ((nvn_cb_t *)c)->pool = pool; ((nvn_cb_t *)c)->pool_off = off; ((nvn_cb_t *)c)->pool_size = n; }
}
static void nvn_cb_addctlmem(void *c, void *m, uint64_t n)
{
    if (c) { ((nvn_cb_t *)c)->ctl = m; ((nvn_cb_t *)c)->ctl_size = n; }
}
static void nvn_cb_begin(void *c) { if (c) ((nvn_cb_t *)c)->recording = 1; }
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

/* ---- 内存池查询:返回池内偏移的宿主指针(命令录制里当作 GPU 地址占位) ---- */
static uint64_t nvn_pool_addr(const void *pool, int64_t off)
{
    const nvn_pool_t *p = (const nvn_pool_t *)pool;
    if (p == NULL) return 0;
    return (uint64_t)(uintptr_t)((uint8_t *)p->mem + (size_t)off);
}
static uint64_t nvn_pool_map(void *pool) { const nvn_pool_t *p = pool; return p ? (uint64_t)(uintptr_t)p->mem : 0; }
static uint64_t nvn_pool_map_virtual(void *pool, int64_t off, uint64_t n) { (void)n; return nvn_pool_addr(pool, off); }
static uint64_t nvn_pool_get_size(const void *pool) { const nvn_pool_t *p = pool; return p ? p->size : 0; }
static void nvn_pool_get_flags(const void *pool, int *out) { (void)pool; if (out) *out = 0; }
static void nvn_pool_flush(void *pool, int64_t off, uint64_t n) { (void)pool; (void)off; (void)n; }
static void nvn_pool_invalidate(void *pool, int64_t off, uint64_t n) { (void)pool; (void)off; (void)n; }
static void nvn_pool_finalize(void *pool) { if (pool) ((nvn_pool_t *)pool)->magic = 0; }

/* ---- 队列/窗口 builder 的查询 ---- */
static uint64_t nvn_qb_get_qmemsz(const void *b)
{
    const nvn_queueb_t *q = b;
    if (q == NULL) return 0;
    return q->cmdsz + q->compsz + q->ctlsz + q->flush;
}
static void *nvn_qb_get_dev(const void *b) { const nvn_queueb_t *q = b; return q ? q->dev : NULL; }
static void nvn_qb_get_flags(const void *b, int *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->flags; }
static void nvn_qb_get_cmdsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->cmdsz; }
static void nvn_qb_get_compsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->compsz; }
static void nvn_qb_get_ctlsz(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->ctlsz; }
static void nvn_qb_get_flush(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->flush; }
static uint64_t nvn_qb_get_memsz(const void *b) { return nvn_qb_get_qmemsz(b); }
static void nvn_qb_get_mem(const void *b, uint64_t *out) { const nvn_queueb_t *q = b; if (q && out) *out = q->qmem; }

/* ---- 队列收尾/present ---- */
static void nvn_queue_flush(void *q) { (void)q; }
static void nvn_queue_finish(void *q) { (void)q; }
static uint32_t nvn_queue_present(void *q, const void *tex) { (void)q; (void)tex; g_nvn_presents++; if (g_nvn_presents <= 5) DMLOG_I("NVN QueuePresentTexture #%{public}llu", (unsigned long long)g_nvn_presents); return 1; }
static uint32_t nvn_queue_acquire(void *q, const void *tex) { (void)q; (void)tex; return 1; }
static uint32_t nvn_window_acquire(void *w, const void *tex) { (void)w; (void)tex; return 1; }
static void nvn_window_finalize(void *w) { if (w) ((nvn_win_t *)w)->magic = 0; }
static void nvn_window_set_interval(void *w, int n) { if (w) ((nvn_win_t *)w)->interval = n; }
static void nvn_window_get_interval(const void *w, int *out) { if (w && out) *out = ((const nvn_win_t *)w)->interval; }
static void nvn_window_get_native(const void *w, void **out) { if (w && out) *out = ((const nvn_win_t *)w)->native; }

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
        return (uint64_t)(uintptr_t)g_fns[id];   /* 已实现/录制入口 */
    }
#ifdef __aarch64__
    return (uint64_t)(uintptr_t)(dm_nvn_thunks + (size_t)id * 8);
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
    if (boot_n < 8) {
        DMLOG_I("NVN bootstrap(%{public}s) -> %{public}#llx", name ? name : "?", (unsigned long long)r);
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
