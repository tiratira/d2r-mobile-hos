#define _GNU_SOURCE 1
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
#include "gfx/tex/nvn_tex.h"
#include "gfx/tex/nvn_deswizzle.h"
#include "gfx/tex/nvn_bcdec.h"
#include "host/dm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern uint64_t dmVkFrames(void);
extern uint64_t dmVkUploadTexture(const void *pixels, uint32_t w, uint32_t h, uint32_t vkfmt, uint32_t bpp);
extern uint64_t dmVkUploadCount(void);
extern uint64_t dmVkLoadShaderModule(const char *path);
extern uint64_t dmVkCreateComputePipeline(const char *path);
extern uint64_t dmVkPipelineCount(void);
extern int dmVkPipelineError(void);
extern int dmVkDslBindings(void);
extern int dmVkShaderStageOfFile(const char *path);
extern uint64_t dmVkCreateGraphicsPipeline(const char *vsPath, const char *psPath);
extern uint64_t dmVkPipelineForKeys(const char *k0, const char *k1);
extern int dmVkPipelineCacheCount(void);
extern int dmVkPipelineObjCount(void);
extern uint64_t dmVkCachedPipeline(const char *k0, const char *k1);
extern uint64_t dmVkReplayDrawCount(void);
extern uint64_t dmVkGraphicsPipeCount(void);
extern int dmVkGraphicsPipeError(void);
extern uint64_t dmVkShaderTestCount(void);

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
/* 默认清屏色故意用绿色:宿主 Vulkan 通路是否在出帧一眼可见 */
static float g_nvn_clear[4] = {0.0f, 0.45f, 0.0f, 1.0f};

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
/* M4 资源表:buffer 对象(<= sizeof(NVNbuffer)=48) */
typedef struct { uint32_t magic; uint32_t pad; uint64_t size; void *pool; int64_t off; } nvn_buf_t;
typedef struct {
    uint32_t magic; uint32_t recording;
    uint64_t used;
    uint8_t *own; size_t own_sz;
    void *dev; const void *pool; int64_t pool_off; uint64_t pool_size;
    void *ctl; uint64_t ctl_size;
    uint64_t handles, ops, clears;
} nvn_cb_t;

static uint32_t g_nvn_devs, g_nvn_queues, g_nvn_wins, g_nvn_cbs, g_nvn_pools;
static uint64_t g_nvn_presents, g_nvn_submits, g_nvn_clears, g_nvn_ops;
static uint64_t g_draws, g_pipe_hits;
/* M4-5:把解析结果发布给渲染线程(PROG=0/DRAW=1),由 dm_vk 在 render pass 内执行 */
typedef struct { int kind; char k0[17], k1[17]; uint32_t a, b; } dm_rp_t;
static dm_rp_t g_rp[4096];
static _Atomic int g_rp_n;
int dmNvnReplayCount(void) { return g_rp_n; }
int dmNvnReplayGet(int i, int *kind, char *k0, char *k1, unsigned *a, unsigned *b)
{
    if (i < 0 || i >= g_rp_n) return 0;
    *kind = g_rp[i].kind;
    memcpy(k0, g_rp[i].k0, 17); memcpy(k1, g_rp[i].k1, 17);
    *a = g_rp[i].a; *b = g_rp[i].b;
    return 1;
}
static void dm_rp_add(int kind, const char *k0, const char *k1, unsigned a, unsigned b)
{
    int n = g_rp_n;
    if (n >= 4096) return;
    g_rp[n].kind = kind;
    if (k0) memcpy(g_rp[n].k0, k0, 17); else g_rp[n].k0[0] = 0;
    if (k1) memcpy(g_rp[n].k1, k1, 17); else g_rp[n].k1[0] = 0;
    g_rp[n].a = a; g_rp[n].b = b;
    g_rp_n = n + 1;
}
typedef struct { char k0[17], k1[17]; } dm_pair_t;
static dm_pair_t g_req[64]; static int g_req_n;
static int dmNvnQueuePair(const char *k0, const char *k1)
{
    if (k0 == NULL || k1 == NULL) return 0;
    for (int i = 0; i < g_req_n; i++)
        if (strcmp(g_req[i].k0, k0) == 0 && strcmp(g_req[i].k1, k1) == 0) return 0;
    if (g_req_n >= 64) return 0;
    snprintf(g_req[g_req_n].k0, 17, "%s", k0);
    snprintf(g_req[g_req_n].k1, 17, "%s", k1);
    g_req_n++;
    g_pipe_hits++;
    return 1;
}
int dmNvnNextRequestedPair(char *k0, char *k1)
{
    static int head;
    if (head >= g_req_n) return 0;
    memcpy(k0, g_req[head].k0, 17); memcpy(k1, g_req[head].k1, 17);
    head++;
    return 1;
}

static uint64_t g_rec_bind2, g_rec_bind3, g_rec_draw3, g_rec_draw4, g_rec_prog;
static int dmNvnProgramKeysOf(void *prog, char *k0, char *k1);
static uint64_t g_nvn_bufs, g_nvn_textures;

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
/* ================= M1:命令流录制 =================
 * 宏:每个 NVN 录制入口写一条定长 op 到游戏给的命令内存(nvnCommandBufferAddCommandMemory
 * 拿到的 pool+off+size),nvnCommandBufferEndRecording 返回该段首地址作为 handle,
 * nvnQueueSubmitCommands 解析 op 流执行(当前先做 clear / 计数,Vulkan draw 回放 M4 接入)。
 */
#define DM_OP_WORDS 8
typedef struct { uint32_t op, n; uint64_t a, b, c, d; float f[4]; } dm_op_t;
enum { DM_OP_END = 0, DM_OP_CLEAR, DM_OP_BARRIER, DM_OP_DRAW, DM_OP_BIND, DM_OP_COPY, DM_OP_DISPATCH, DM_OP_PROG };
static nvn_cb_t *g_cur_cb;
static uint64_t g_addcmd_n, g_begin_n, g_end_n, g_cbinit_n;
static void *g_addcmd_cb[4]; static void *g_addcmd_pool[4]; static long long g_addcmd_off[4]; static unsigned long long g_addcmd_size[4];
static uint64_t g_begin_alias_n;
static void *g_bind_sample[8]; static int g_bind_sample_n; static uint64_t g_bind_lookup_miss;
static void *g_alias_begin[8]; static void *g_alias_match[8]; static int g_alias_n;
static void *g_cb_ev[64]; static uint8_t g_cb_kind[64]; static int g_cb_ev_n;
static void dm_cb_ev(void *cb, int kind) { if (g_cb_ev_n < 64) { g_cb_ev[g_cb_ev_n] = cb; g_cb_kind[g_cb_ev_n] = (uint8_t)kind; g_cb_ev_n++; } }
static void *g_last_base; static uint64_t g_last_used; static void *g_last_pool;
static uint64_t g_dm_ops;
static uint32_t g_op_hist[16];

static void dm_op_write4(nvn_cb_t *cb, uint32_t op, uint64_t a, uint64_t b, uint64_t c, uint64_t d);
static uint8_t *dm_cb_stream(nvn_cb_t *cb, size_t need)
{
    if (cb == NULL) return NULL;
    const nvn_pool_t *pl = (const nvn_pool_t *)cb->pool;
    if (pl != NULL && pl->mem != NULL) return (uint8_t *)pl->mem + (size_t)cb->pool_off;
    /* 没有命令内存(引擎大量 CB 如此):用宿主自有缓冲,保证 op 流可回放 */
    if (cb->own == NULL || cb->own_sz < need) {
        size_t nsz = need > 65536 ? need : 65536;
        uint8_t *nb = (uint8_t *)realloc(cb->own, nsz);
        if (nb == NULL) return NULL;
        cb->own = nb; cb->own_sz = nsz;
    }
    return cb->own;
}
static void dm_op_write(nvn_cb_t *cb, uint32_t op, const void *p, size_t n)
{
    if (cb == NULL) return;
    uint8_t *base = dm_cb_stream(cb, (size_t)cb->used + sizeof(dm_op_t) + 8);
    if (base == NULL) return;
    size_t need = sizeof(dm_op_t);
    size_t cur = (size_t)cb->used;
    if (cur + need > (size_t)(cb->pool ? cb->pool_size : cb->own_sz)) return;
    dm_op_t *o = (dm_op_t *)(base + cur);
    memset(o, 0, sizeof(*o));
    o->op = op; o->n = (uint32_t)n;
    if (n) memcpy(o->f, p, n);
    cb->used += need;
    g_dm_ops++;
    if (op < 16) g_op_hist[op]++;
}
static void dm_op_write4(nvn_cb_t *cb, uint32_t op, uint64_t a, uint64_t b, uint64_t c, uint64_t d)
{
    if (cb == NULL) return;
    uint8_t *base4 = dm_cb_stream(cb, (size_t)cb->used + sizeof(dm_op_t) + 8);
    if (base4 == NULL) return;
    size_t cur = (size_t)cb->used;
    if (cur + sizeof(dm_op_t) > (size_t)(cb->pool ? cb->pool_size : cb->own_sz)) return;
    dm_op_t *o = (dm_op_t *)(base4 + cur);
    memset(o, 0, sizeof(*o));
    o->op = op; o->a = a; o->b = b; o->c = c; o->d = d;
    cb->used += sizeof(dm_op_t);
    g_dm_ops++;
    if (op < 16) g_op_hist[op]++;
}
#define DM_REC(name, opcode, argn) \
    static uint64_t rec_##name(uint64_t a, uint64_t b, uint64_t c, uint64_t d) \
    { dm_op_write(g_cur_cb, opcode, (double[]){0}, 0); (void)a; (void)b; (void)c; (void)d; return 0; }
/* 具体 op 记录:签名统一 4 个 uint64,多余参数按 ABI 忽略 */
static uint64_t op_setviewport(uint64_t cb, uint64_t a, uint64_t b, uint64_t c) { dm_op_write((nvn_cb_t *)cb, DM_OP_BIND, NULL, 0); (void)a;(void)b;(void)c; return 0; }
static uint64_t op_draw3(uint64_t cb, uint64_t a, uint64_t b, uint64_t c) { g_rec_draw3++; dm_op_write4((nvn_cb_t *)cb, DM_OP_DRAW, a, b, c, 0); return 0; }
static uint64_t op_draw4(uint64_t cb, uint64_t a, uint64_t b, uint64_t c, uint64_t d) { g_rec_draw4++; dm_op_write4((nvn_cb_t *)cb, DM_OP_DRAW, a, b, c, d); return 0; }
static uint64_t op_bind2(uint64_t cb, uint64_t a, uint64_t b) { g_rec_bind2++; dm_op_write4((nvn_cb_t *)cb, DM_OP_BIND, a, b, 0, 0); return 0; }
static uint64_t op_bind_prog(uint64_t cb, uint64_t prog) { g_rec_prog++; dm_op_write4((nvn_cb_t *)cb, DM_OP_PROG, cb, prog, 0, 0); return 0; }
static uint64_t op_bind3(uint64_t cb, uint64_t a, uint64_t b, uint64_t c) { g_rec_bind3++; dm_op_write4((nvn_cb_t *)cb, DM_OP_BIND, a, b, c, 0); return 0; }
static uint64_t op_copy(uint64_t cb, uint64_t a, uint64_t b, uint64_t c, uint64_t d) { dm_op_write((nvn_cb_t *)cb, DM_OP_COPY, NULL, 0); (void)a;(void)b;(void)c;(void)d; return 0; }
static uint64_t op_dispatch(uint64_t cb, uint64_t a, uint64_t b, uint64_t c) { dm_op_write((nvn_cb_t *)cb, DM_OP_DISPATCH, NULL, 0); (void)a;(void)b;(void)c; return 0; }



static void dm_dump_submit(int n, const uint64_t *h, uint64_t *outbuf, int maxw);
static void nvn_queue_submit(void *q, int n, const void *handles)
{
    const uint64_t *h = (const uint64_t *)handles;
    {
        static int dumped;
        if (!dumped && h != NULL && n > 0) {
            dumped = 1;
            uint64_t words[8 * 4];
            dm_dump_submit(n, h, words, 8);
        }
    }
    g_rp_n = 0;
    if (g_nvn_submits <= 5) DMLOG_I("NVN queue submit q=%{public}llx n=%{public}d", (unsigned long long)(uintptr_t)q, n);
    for (int i = 0; i < n; i++) {
        const dm_op_t *o = (const dm_op_t *)(uintptr_t)h[i];
        for (int k = 0; k < 4096 && o != NULL && o->op != DM_OP_END; k++, o++) {
            if (o->op == DM_OP_CLEAR) {
                dmVkSetClearColor(o->f[0], o->f[1], o->f[2], o->f[3]);
            } else if (o->op == DM_OP_BIND && o->b != 0) {
                char k0[17], k1[17];
                if (dmNvnProgramKeysOf((void *)(uintptr_t)o->b, k0, k1)) {
                    (void)dmNvnQueuePair(k0, k1);
                } else {
                    g_bind_lookup_miss++;
                    if (g_bind_sample_n < 8) { g_bind_sample[g_bind_sample_n] = (void *)(uintptr_t)o->b; g_bind_sample_n++; }
                }
            } else if (o->op == DM_OP_PROG) {
                char k0[17], k1[17];
                if (dmNvnProgramKeysOf((void *)(uintptr_t)o->b, k0, k1)) {
                    (void)dmNvnQueuePair(k0, k1);
                    dm_rp_add(0, k0, k1, 0, 0);
                }
            } else if (o->op == DM_OP_DRAW) {
                g_draws++;
                dm_rp_add(1, NULL, NULL, (unsigned)o->a, (unsigned)o->b);
            }
        }
    }
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
    g_cbinit_n++; dm_cb_ev(c, 0);
    if (g_nvn_cbs < 8) {
        DMLOG_I("NVN cb init c=%{public}llx dev=%{public}llx", (unsigned long long)(uintptr_t)c, (unsigned long long)(uintptr_t)d);
    }
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
    g_addcmd_n++; dm_cb_ev(c, 1);
    if (g_addcmd_n <= 4) { int k = (int)g_addcmd_n - 1; g_addcmd_cb[k] = c; g_addcmd_pool[k] = (void *)pool; g_addcmd_off[k] = (long long)off; g_addcmd_size[k] = (unsigned long long)n; }
    DMLOG_I("NVN cb addcmd pool=%{public}llx off=%{public}lld n=%{public}llu", (unsigned long long)(uintptr_t)pool, (long long)off, (unsigned long long)n);
    if (c) { ((nvn_cb_t *)c)->pool = pool; ((nvn_cb_t *)c)->pool_off = off; ((nvn_cb_t *)c)->pool_size = n; }
}
static void nvn_cb_addctlmem(void *c, void *m, uint64_t n)
{
    if (c) { ((nvn_cb_t *)c)->ctl = m; ((nvn_cb_t *)c)->ctl_size = n; }
}
static void nvn_cb_begin(void *c)
{
    g_begin_n++; dm_cb_ev(c, 2);
    if (c == NULL) return;
    {
        int matched = 0;
        for (int i = 0; i < 4; i++) if (g_addcmd_cb[i] == c) matched = 1;
        if (!matched && g_addcmd_n > 0) {
            g_begin_alias_n++;
            if (g_alias_n < 8) { g_alias_begin[g_alias_n] = c; g_alias_match[g_alias_n] = g_addcmd_cb[0]; g_alias_n++; }
        }
    }
    ((nvn_cb_t *)c)->recording = 1;
    ((nvn_cb_t *)c)->used = 0;
    g_cur_cb = (nvn_cb_t *)c;
    if (g_nvn_cbs < 8) {
        DMLOG_I("NVN cb begin c=%{public}llx", (unsigned long long)(uintptr_t)c);
    }
}
static uint64_t nvn_cb_end(void *c)
{
    g_end_n++; dm_cb_ev(c, 3);
    nvn_cb_t *o = (nvn_cb_t *)c;
    if (o == NULL) return 0;
    o->recording = 0;
    /* NVNcommandHandle 是「命令内存里的地址」,不是自增计数;引擎会拿着它当
     * 指针/地址用。把一条 8 字节命令头写进游戏给的命令内存,返回其地址。 */
    uint8_t *base = dm_cb_stream(o, (size_t)o->used + sizeof(dm_op_t) + 8);
    if (base != NULL) {
        dm_op_t *e = (dm_op_t *)(base + (size_t)o->used);
        size_t cap = (size_t)(o->pool ? o->pool_size : o->own_sz);
        if ((size_t)o->used + sizeof(dm_op_t) <= cap) {
            memset(e, 0, sizeof(*e));
            e->op = DM_OP_END;
            o->used += sizeof(dm_op_t);
        }
    }
    /* 句柄 = 本段命令流首地址(固定),Submit 从这里顺序解析 op */
    g_last_base = base; g_last_used = (uint64_t)o->used; g_last_pool = o->pool;
    o->handles++;
    g_cur_cb = NULL;
    if (o->handles <= 2) {
        DMLOG_I("NVN cb end h=%{public}llx", (unsigned long long)(uintptr_t)base);
    }
    return (uint64_t)(uintptr_t)base;
}
static uint32_t nvn_cb_isrec(const void *c) { return c ? ((const nvn_cb_t *)c)->recording : 0; }
static void nvn_cb_clear_color(void *c, int idx, const float *rgba, int mask)
{
    (void)idx; (void)mask;
    if (rgba != NULL && c != NULL && ((nvn_cb_t *)c)->recording) {
        dm_op_write((nvn_cb_t *)c, DM_OP_CLEAR, rgba, 16);
    }
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
static uint32_t nvn_buffer_init(void *b, void *d, uint64_t flags) { (void)flags; g_nvn_bufs++; return nvn_generic_init(b, d, 0x4655424eU); }

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
/* NVNformat -> 每像素字节数 / 压缩格式块大小(见 nvn.h 的 NVN_FORMAT_*)。
 * D2R 大量用 BC1/BC5/BC7:压缩格式若按 4B/px 估,存储会被高估 4~8 倍,
 * 内存池很快耗尽,引擎的 StandardAllocator 会返回 NULL。 */
static unsigned nvn_format_bpp(unsigned f)
{
    if (f >= 0x1 && f <= 0x4) return 1;      /* R8 */
    if (f >= 0x5 && f <= 0x9) return 2;      /* R16 */
    if (f >= 0xA && f <= 0xC) return 4;      /* R32 */
    if (f >= 0xD && f <= 0x10) return 2;     /* RG8 */
    if (f >= 0x11 && f <= 0x15) return 4;    /* RG16 */
    if (f >= 0x16 && f <= 0x18) return 8;    /* RG32 */
    if (f >= 0x19 && f <= 0x1C) return 3;    /* RGB8 */
    if (f >= 0x1D && f <= 0x21) return 6;    /* RGB16 */
    if (f >= 0x22 && f <= 0x24) return 12;   /* RGB32 */
    if (f >= 0x25 && f <= 0x28) return 4;    /* RGBA8 */
    if (f >= 0x29 && f <= 0x2D) return 8;    /* RGBA16 */
    if (f >= 0x2E && f <= 0x30) return 16;   /* RGBA32 */
    switch (f) {
    case 0x31: return 1;                     /* STENCIL8 */
    case 0x32: return 2;                     /* DEPTH16 */
    case 0x33: return 4;                     /* DEPTH24 */
    case 0x34: return 4;                     /* DEPTH32F */
    case 0x35: return 4;                     /* DEPTH24_STENCIL8 */
    case 0x36: return 8;                     /* DEPTH32F_STENCIL8 */
    case 0x37: case 0x38: return 4;          /* RGBX8/RGBA8 sRGB */
    case 0x39: case 0x3A: case 0x3B: case 0x3C: return 2;  /* 4/5/565 */
    case 0x3D: case 0x3E: return 4;          /* RGB10A2 */
    case 0x3F: case 0x40: return 4;          /* R11G11B10F/RGB9E5F */
    case 0x69: case 0x6A: case 0x6B: case 0x6C: return 4;  /* RGB10A2 variants */
    case 0x6D: return 4;                     /* RGBX8 */
    default: break;
    }
    if (f >= 0x51 && f <= 0x6C) {             /* *_UI2F/*_I2F:同上但整体偏移 0x50 */
        return nvn_format_bpp(f - 0x50);
    }
    return 4;
}
/* 压缩格式:返回每 4x4 块的字节数;0 = 非压缩 */
static unsigned nvn_format_block(unsigned f)
{
    switch (f) {
    case 0x41: case 0x42: case 0x45: case 0x46:    /* DXT1 (BC1) 8B/块 */
    case 0x49: case 0x4A:                          /* RGTC1 8B/块 */
        return 8;
    case 0x43: case 0x44: case 0x47: case 0x48:    /* DXT3/DXT5 */
    case 0x4B: case 0x4C: case 0x4D: case 0x4E:    /* RGTC2/BPTC */
    case 0x4F: case 0x50:                          /* BPTC float */
        return 16;
    default:
        return 0;
    }
}
static uint64_t nvn_texb_size(const void *b)
{
    const nvn_texb_t *t = (const nvn_texb_t *)b;
    if (t == NULL) return 0;
    uint64_t w = t->width > 0 ? (uint64_t)t->width : 1;
    uint64_t h = t->height > 0 ? (uint64_t)t->height : 1;
    uint64_t depth = t->depth > 0 ? (uint64_t)t->depth : 1;
    uint64_t levels = t->levels > 0 ? (uint64_t)t->levels : 1;
    unsigned blk = nvn_format_block(t->format);
    unsigned bpp = nvn_format_bpp(t->format);
    uint64_t bytes = 0;
    for (uint64_t l = 0; l < levels; l++) {
        uint64_t lw = w >> l; if (lw == 0) lw = 1;
        uint64_t lh = h >> l; if (lh == 0) lh = 1;
        if (blk != 0) {
            bytes += ((lw + 3) / 4) * ((lh + 3) / 4) * blk * depth;
        } else {
            bytes += lw * lh * depth * bpp;
        }
    }
    uint64_t aligned = (bytes + 4095) & ~(uint64_t)4095;
    DMLOG_I("NVN texb size %{public}dx%{public}dx%{public}d lv=%{public}d fmt=%{public}u bpp=%{public}u blk=%{public}u -> %{public}llu",
            t->width, t->height, t->depth, t->levels, t->format, bpp, blk, (unsigned long long)aligned);
    return aligned;
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


/* ---- 纹理对象:GetStorageSize/GetViewOffset 都是直接返回值(nvn.h) ---- */
typedef struct { uint32_t magic; uint32_t pad; uint64_t size; uint64_t align; void *dev;
    nvn_texdesc desc; } nvn_tex_t;
static uint64_t nvn_buffer_get_size(const void *b)
{
    return b ? ((const nvn_buf_t *)b)->size : 0;
}

static uint32_t nvn_texture_init2(void *t, const void *b)
{
    if (t == NULL) return 0;
    nvn_tex_t *o = (nvn_tex_t *)t;
    memset(o, 0, sizeof(*o));
    o->magic = 0x58544554U;
    o->size = nvn_texb_size(b);
    o->align = 4096;
    g_nvn_textures++;
    if (b != NULL) {
        const nvn_texb_t *tb = (const nvn_texb_t *)b;
        o->desc.target = (int)tb->target;
        o->desc.format = (int)tb->format;
        o->desc.samples = tb->samples ? tb->samples : 1;
        o->desc.width = tb->width > 0 ? (uint32_t)tb->width : 1;
        o->desc.height = tb->height > 0 ? (uint32_t)tb->height : 1;
        o->desc.depth = tb->depth > 0 ? (uint32_t)tb->depth : 1;
        o->desc.levels = tb->levels > 0 ? (uint32_t)tb->levels : 1;
    }
    return 1;
}
static int nvn_texture_get_storage_size(const void *t)
{
    const nvn_tex_t *o = (const nvn_tex_t *)t;
    return o ? (int)o->size : 0;
}
static int64_t nvn_texture_get_view_offset(const void *t, const void *view)
{
    (void)t; (void)view;
    return 0;
}


/* 其余 texture builder setter/getter:引擎会用 SetSize2D 等,补上才能算出正确
 * GetStorageSize(此前只 SetWidth/Height 时尺寸对,但别的路径可能用 Size2D)。 */
static void nvn_texb_set1d(void *b, int w) { if (b) { ((nvn_texb_t *)b)->width = w; ((nvn_texb_t *)b)->height = 1; ((nvn_texb_t *)b)->depth = 1; } }
static void nvn_texb_set2d(void *b, int w, int h) { if (b) { ((nvn_texb_t *)b)->width = w; ((nvn_texb_t *)b)->height = h; ((nvn_texb_t *)b)->depth = 1; } }
static void nvn_texb_set3d(void *b, int w, int h, int d) { if (b) { ((nvn_texb_t *)b)->width = w; ((nvn_texb_t *)b)->height = h; ((nvn_texb_t *)b)->depth = d; } }
static void nvn_texb_setswizzle(void *b, int r, int g, int bl, int a) { (void)b; (void)r; (void)g; (void)bl; (void)a; }
static void nvn_texb_setdsmode(void *b, int m) { (void)b; (void)m; }
static void nvn_texb_setstride(void *b, int st) { (void)b; (void)st; }
static void nvn_texb_setstorage(void *b, void *pool, int64_t off) { (void)b; (void)pool; (void)off; }
static void nvn_texb_setpkgdata(void *b, void *d) { (void)b; (void)d; }
static void nvn_texb_setpkglayout(void *b, int l) { (void)b; (void)l; }
static void nvn_texb_setgln(void *b, unsigned n) { (void)b; (void)n; }
static int nvn_texb_gettarget(const void *b) { return b ? (int)((const nvn_texb_t *)b)->target : 0; }
static int nvn_texb_getwidth(const void *b) { return b ? ((const nvn_texb_t *)b)->width : 0; }
static int nvn_texb_getheight(const void *b) { return b ? ((const nvn_texb_t *)b)->height : 0; }
static int nvn_texb_getdepth(const void *b) { return b ? ((const nvn_texb_t *)b)->depth : 0; }
static int nvn_texb_getlevels(const void *b) { return b ? ((const nvn_texb_t *)b)->levels : 0; }
static int nvn_texb_getformat(const void *b) { return b ? (int)((const nvn_texb_t *)b)->format : 0; }
static int nvn_texb_getsamples(const void *b) { return b ? ((const nvn_texb_t *)b)->samples : 0; }
static int nvn_texb_getstoreclass(const void *b) { (void)b; return 0; }
static int nvn_texb_getdsmode(const void *b) { (void)b; return 0; }
static int nvn_texb_getswizzle(const void *b) { (void)b; return 0; }
static uint64_t nvn_texb_getpkgdata(const void *b) { (void)b; return 0; }
static int nvn_texb_getpkglayout(const void *b) { (void)b; return 0; }
static int nvn_texb_getstride(const void *b) { (void)b; return 0; }
static int nvn_texb_getglname(const void *b) { (void)b; return 0; }
static int nvn_texb_getzull(const void *b) { (void)b; return 0; }
static uint64_t nvn_texb_getpool(const void *b) { (void)b; return 0; }
static int64_t nvn_texb_getoff(const void *b) { (void)b; return 0; }
static int nvn_texb_getrawclass(const void *b) { (void)b; return 0; }


/* ================= M4-3a:程序 → 着色器键(sha256 控制段前 16 位) =================
 * nvn.h: nvnProgramSetShaders(NVNprogram*, int n, const NVNshaderData *sd),
 * NVNshaderData = { const void *data; size_t size; }。键与 shader_map.tsv 第 4 列、
 * 以及离线 SPIR-V 的文件名一致,回放层据此找 <key>.spv/.res.json。 */
/* nvn.h: NVNshaderData = { NVNbufferAddress data; const void* control; } */
typedef struct { uint64_t data; const void *control; } nvn_shader_data_t;
static char g_prog_keys[512][17];
static int g_prog_key_n;
typedef struct { void *prog; char k0[17], k1[17]; } dm_progent_t;
static dm_progent_t g_progtab[1024];
static int g_progtab_n;
static int dmNvnProgramKeysOf(void *prog, char *k0, char *k1)
{
    for (int i = 0; i < g_progtab_n; i++)
        if (g_progtab[i].prog == prog) { memcpy(k0, g_progtab[i].k0, 17); memcpy(k1, g_progtab[i].k1, 17); return 1; }
    return 0;
}
static char g_prog_lines[512][64];
static int g_prog_line_n;
static int g_progs;
static int g_keys_loaded, g_cspipes, g_real_gpipes;

static void sha256_hex16(const void *in, size_t len, char out[17]);
static uint32_t nvn_program_setshaders(void *prog, int n, const void *sdv)
{
    if (n < 0 || n > 6 || sdv == NULL) return 0;
    const nvn_shader_data_t *sd = (const nvn_shader_data_t *)sdv;
    char line[64] = {0};
    int keys_here = 0;
    for (int i = 0; i < n && g_prog_key_n < 512; i++) {
        const uint8_t *c = (const uint8_t *)sd[i].control;
        if (c == NULL || ((uintptr_t)c & 3)) continue;
        sha256_hex16(c, 0x880, g_prog_keys[g_prog_key_n]);
        if (keys_here < 2) {
            size_t l = strlen(line);
            if (l && l + 18 < sizeof(line)) { line[l] = ' '; line[l+1] = 0; l++; }
            memcpy(line + l, g_prog_keys[g_prog_key_n], 16);
            line[l + 16] = 0;
            keys_here++;
        }
        g_prog_key_n++;
    }
    if (keys_here >= 1) {
        char a[32] = {0}, b[32] = {0};
        int got = sscanf(line, "%31s %31s", a, b);
        dm_progent_t *e = NULL;
        for (int i = 0; i < g_progtab_n; i++) if (g_progtab[i].prog == prog) { e = &g_progtab[i]; break; }
        if (e == NULL && g_progtab_n < 1024) { e = &g_progtab[g_progtab_n++]; e->prog = prog; e->k0[0] = 0; e->k1[0] = 0; }
        if (e != NULL) {
            if (got >= 1) memcpy(e->k0, a, 17);
            if (got >= 2) memcpy(e->k1, b, 17);
        }
    }
    if (keys_here && g_prog_line_n < 512) {
        snprintf(g_prog_lines[g_prog_line_n], sizeof(g_prog_lines[0]), "%s", line);
        g_prog_line_n++;
    }
    g_progs++;
    return 1;
}

/* ---- 紧凑 SHA-256(公开算法实现) ---- */
static const uint32_t K256[64] = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR32(x,n) (((x) >> (n)) | ((x) << (32-(n))))
static void sha256_hex16(const void *in, size_t len, char out[17])
{
    const uint8_t *p = (const uint8_t *)in;
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint32_t w[64];
    size_t i = 0;
    while (i + 64 <= len) {
        for (int k = 0; k < 16; k++)
            w[k] = ((uint32_t)p[i+k*4]<<24)|((uint32_t)p[i+k*4+1]<<16)|((uint32_t)p[i+k*4+2]<<8)|p[i+k*4+3];
        i += 64;
        for (int k = 16; k < 64; k++) {
            uint32_t s0 = ROR32(w[k-15],7) ^ ROR32(w[k-15],18) ^ (w[k-15] >> 3);
            uint32_t s1 = ROR32(w[k-2],17) ^ ROR32(w[k-2],19) ^ (w[k-2] >> 10);
            w[k] = w[k-16] + s0 + w[k-7] + s1;
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int k = 0; k < 64; k++) {
            uint32_t S1 = ROR32(e,6) ^ ROR32(e,11) ^ ROR32(e,25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + K256[k] + w[k];
            uint32_t S0 = ROR32(a,2) ^ ROR32(a,13) ^ ROR32(a,22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    /* 末尾补齐:0x80 + 0 + 64 位长度(恰好 64 字节时也要多一个块) */
    uint8_t pad[128];
    size_t rem = len - i;
    memset(pad, 0, sizeof(pad));
    if (rem) memcpy(pad, p + i, rem);
    pad[rem] = 0x80;
    size_t padlen = (rem < 56) ? 64 : 128;
    uint64_t bits = (uint64_t)len * 8;
    for (int k = 0; k < 8; k++) pad[padlen - 1 - k] = (uint8_t)(bits >> (8 * k));
    for (size_t off = 0; off < padlen; off += 64) {
        for (int k = 0; k < 16; k++)
            w[k] = ((uint32_t)pad[off+k*4]<<24)|((uint32_t)pad[off+k*4+1]<<16)|((uint32_t)pad[off+k*4+2]<<8)|pad[off+k*4+3];
        for (int k = 16; k < 64; k++) {
            uint32_t s0 = ROR32(w[k-15],7) ^ ROR32(w[k-15],18) ^ (w[k-15] >> 3);
            uint32_t s1 = ROR32(w[k-2],17) ^ ROR32(w[k-2],19) ^ (w[k-2] >> 10);
            w[k] = w[k-16] + s0 + w[k-7] + s1;
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int k = 0; k < 64; k++) {
            uint32_t S1 = ROR32(e,6) ^ ROR32(e,11) ^ ROR32(e,25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + K256[k] + w[k];
            uint32_t S0 = ROR32(a,2) ^ ROR32(a,13) ^ ROR32(a,22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+S0+mj;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    static const char hexd[] = "0123456789abcdef";
    for (int q = 0; q < 8; q++)
        for (int k = 0; k < 4; k++) {
            uint8_t byte = (uint8_t)(h[q] >> (24 - 8*k));
            out[q*8 + k*2] = hexd[byte >> 4];
            out[q*8 + k*2 + 1] = hexd[byte & 15];
        }
    out[16] = 0;
}


/* 状态看门狗:热路径 trace 会刷爆 hilog,这里每 5 s 打一次累计计数,
 * 便于判断 present/submit 是否真的在发生。 */
static void *nvn_stats_thread(void *arg)
{
    (void)arg;
    int up_tested = 0;
    for (;;) {
        sleep(2);
        if (!up_tested && dmVkFrames() > 0) {
            up_tested = 1;
            static uint8_t px[64 * 64 * 4];
            for (int i = 0; i < 64 * 64; i++) {
                px[i * 4 + 0] = (uint8_t)i; px[i * 4 + 1] = (uint8_t)(i >> 6);
                px[i * 4 + 2] = 0x40; px[i * 4 + 3] = 0xff;
            }
            uint64_t h = dmVkUploadTexture(px, 64, 64, 37 /* VK_FORMAT_R8G8B8A8_UNORM */, 4);
            {
                FILE *kf = fopen("/data/storage/el2/base/haps/entry/files/dm_keys.txt", "r");
                if (kf != NULL) {
                    char line[64];
                    while (fgets(line, sizeof line, kf) != NULL && g_keys_loaded < 64) {
                        char *nl = strchr(line, '\n'); if (nl) *nl = 0;
                        if (line[0] == 0) continue;
                        char path[256];
                        snprintf(path, sizeof path, "/data/storage/el2/base/haps/entry/files/shaders/%s.spv", line);
                        int st = dmVkShaderStageOfFile(path);
                        if (st < 0) continue;
                        uint64_t m = dmVkLoadShaderModule(path);
                        if (m) g_keys_loaded++;
                        if (st == 5) { if (dmVkCreateComputePipeline(path)) g_cspipes++; }
                    }
                    fclose(kf);
                }
                FILE *pf2 = fopen("/data/storage/el2/base/haps/entry/files/dm_progs.txt", "r");
                if (pf2 != NULL) {
                    char pl[64]; int tried = 0;
                    while (fgets(pl, sizeof pl, pf2) != NULL && tried < 16) {
                        char k0[32] = {0}, k1[32] = {0};
                        if (sscanf(pl, "%31s %31s", k0, k1) != 2) continue;
                        char p0[256], p1[256];
                        snprintf(p0, sizeof p0, "/data/storage/el2/base/haps/entry/files/shaders/%s.spv", k0);
                        snprintf(p1, sizeof p1, "/data/storage/el2/base/haps/entry/files/shaders/%s.spv", k1);
                        if (dmVkPipelineForKeys(k0, k1)) { g_real_gpipes++; }
                        tried++;
                    }
                    fclose(pf2);
                }
                DMLOG_I("M4 shader keys loaded=%d cspipes=%d real_gpipes=%d", g_keys_loaded, g_cspipes, g_real_gpipes, dmVkPipelineCacheCount(), dmVkPipelineObjCount(),
                    (unsigned long long)g_draws, (unsigned long long)g_pipe_hits,
                    (unsigned long long)g_rec_bind2, (unsigned long long)g_rec_bind3,
                    (unsigned long long)(g_rec_draw3 + g_rec_draw4), (unsigned long long)g_rec_prog, (unsigned long long)dmVkReplayDrawCount(), g_real_gpipes);
            }
            DMLOG_I("M2 tex upload self-test h=%{public}llx count=%{public}llu",
                    (unsigned long long)h, (unsigned long long)dmVkUploadCount());
        }
        /* hilog 在热路径下轮转很快,计数落盘更可靠 */
        FILE *f = fopen("/data/storage/el2/base/haps/entry/files/dm_stats.txt", "w");
        if (f != NULL) {
            fprintf(f, "progs=%d keys=%d ", g_progs, g_prog_key_n);
            fprintf(f, "present=%llu submit=%llu clear=%llu ops=%llu vk=%llu tex=%llu bufs=%llu texs=%llu shader=%llu kload=%d cspipe=%d rgpipe=%d pcache=%d pobj=%d draws=%llu pbh=%llu recb2=%llu recb3=%llu recdl=%llu recprog=%llu rpdraw=%llu pipe=%llu pipe_err=%d dsl=%d gpipe=%llu gpipe_err=%d\n",
                    (unsigned long long)g_nvn_presents, (unsigned long long)g_nvn_submits,
                    (unsigned long long)g_nvn_clears, (unsigned long long)g_nvn_ops,
                    (unsigned long long)dmVkFrames(), (unsigned long long)dmVkUploadCount(),
                    (unsigned long long)g_nvn_bufs, (unsigned long long)g_nvn_textures,
                    (unsigned long long)dmVkShaderTestCount(), g_keys_loaded, g_cspipes, g_real_gpipes, dmVkPipelineCacheCount(), dmVkPipelineObjCount(),
                    (unsigned long long)g_draws, (unsigned long long)g_pipe_hits,
                    (unsigned long long)g_rec_bind2, (unsigned long long)g_rec_bind3,
                    (unsigned long long)(g_rec_draw3 + g_rec_draw4), (unsigned long long)g_rec_prog, (unsigned long long)dmVkReplayDrawCount(),
                    (unsigned long long)dmVkPipelineCount(), dmVkPipelineError(), dmVkDslBindings(),
                    (unsigned long long)dmVkGraphicsPipeCount(), dmVkGraphicsPipeError());
            fclose(f);
            {
                static int bind_dumped;
                if (!bind_dumped) {
                    bind_dumped = 1;
                    FILE *bf = fopen("/data/storage/el2/base/haps/entry/files/dm_bind.txt", "w");
                    if (bf != NULL) {
                        fprintf(bf, "progtab=%d bind_miss=%llu\n", g_progtab_n, (unsigned long long)g_bind_lookup_miss);
                        for (int i = 0; i < g_progtab_n && i < 8; i++)
                            fprintf(bf, "prog[%d]=%llx k0=%s k1=%s\n", i, (unsigned long long)(uintptr_t)g_progtab[i].prog, g_progtab[i].k0, g_progtab[i].k1);
                        for (int i = 0; i < g_bind_sample_n; i++)
                            fprintf(bf, "bind[%d]=%llx\n", i, (unsigned long long)(uintptr_t)g_bind_sample[i]);
                        fclose(bf);
                    }
                }
                static int cb_dumped;
                if (!cb_dumped) {
                    cb_dumped = 1;
                    FILE *cf = fopen("/data/storage/el2/base/haps/entry/files/dm_cb.txt", "w");
                    if (cf != NULL) {
                        fprintf(cf, "init=%llu addcmd=%llu begin=%llu end=%llu last_base=%llx last_used=%llu last_pool=%llx\n",
                                (unsigned long long)g_cbinit_n,
                                (unsigned long long)g_addcmd_n, (unsigned long long)g_begin_n, (unsigned long long)g_end_n,
                                (unsigned long long)(uintptr_t)g_last_base, (unsigned long long)g_last_used,
                                (unsigned long long)(uintptr_t)g_last_pool);
                        for (int i = 0; i < (int)(g_addcmd_n > 4 ? 4 : g_addcmd_n); i++)
                            fprintf(cf, "addcmd[%d] cb=%llx pool=%llx off=%lld size=%llu\n", i,
                                    (unsigned long long)(uintptr_t)g_addcmd_cb[i],
                                    (unsigned long long)(uintptr_t)g_addcmd_pool[i],
                                    g_addcmd_off[i], g_addcmd_size[i]);
                        fprintf(cf, "begin_alias=%llu\n", (unsigned long long)g_begin_alias_n);
                        for (int i = 0; i < g_alias_n; i++)
                            fprintf(cf, "alias begin=%llx ref=%llx delta=%lld\n", (unsigned long long)(uintptr_t)g_alias_begin[i],
                                    (unsigned long long)(uintptr_t)g_alias_match[i],
                                    (long long)((intptr_t)g_alias_begin[i] - (intptr_t)g_alias_match[i]));
                        for (int i = 0; i < g_cb_ev_n; i++)
                            fprintf(cf, "ev kind=%u cb=%llx\n", g_cb_kind[i], (unsigned long long)(uintptr_t)g_cb_ev[i]);
                        fclose(cf);
                    }
                }
            }
            if (g_prog_key_n > 0) {
                FILE *kf = fopen("/data/storage/el2/base/haps/entry/files/dm_keys.txt", "w");
                if (kf != NULL) { for (int i = 0; i < g_prog_key_n; i++) fprintf(kf, "%s\n", g_prog_keys[i]); fclose(kf); }
                if (g_prog_line_n > 512) g_prog_line_n = 512;   /* 防御:计数被写坏时不要写巨量文件 */
                static int progs_dumped;
                if (!progs_dumped) {
                    progs_dumped = 1;
                    FILE *pf = fopen("/data/storage/el2/base/haps/entry/files/dm_progs.txt", "w");
                    if (pf != NULL) { for (int i = 0; i < g_prog_line_n; i++) fprintf(pf, "%s\n", g_prog_lines[i]); fclose(pf); }
                }
            }
        }
    }
    return NULL;
}



/* M2 资源层:把一张已初始化纹理的存储按 Tegra 块线性解到线性缓冲(回放上传前用)。
 * dst_pitch 传 0 时按本级块宽自动计算;返回 0 成功,-1 不支持。 */
int dmNvnTextureLinearize(const void *tex, unsigned level, unsigned layer,
                          const void *src_storage, void *dst, size_t dst_pitch)
{
    const nvn_tex_t *t = (const nvn_tex_t *)tex;
    if (t == NULL || src_storage == NULL || dst == NULL) return -1;
    if (dst_pitch == 0) {
        uint32_t rb = 0, rows = 0, slices = 0, bpb = 0; uint64_t base = 0; unsigned th = 0, td = 0;
        if (nvn_deswizzle_geom(&t->desc, level, layer, &rb, &rows, &slices, &base, &th, &td, &bpb) != 0) return -1;
        dst_pitch = rb;
    }
    return nvn_deswizzle(&t->desc, level, layer, src_storage, dst, dst_pitch);
}



/* 诊断:安全读取 handle 指向的前 maxw 个字(process_vm_readv,避免坏指针崩溃),
 * 与录制端的 base/used/pool 一并落盘。 */
#include <sys/uio.h>
static void dm_dump_submit(int n, const uint64_t *h, uint64_t *outbuf, int maxw)
{
    FILE *f = fopen("/data/storage/el2/base/haps/entry/files/dm_parse.txt", "w");
    if (f == NULL) return;
    int lim = n > 4 ? 4 : n;
    fprintf(f, "n=%d last_base=%llx last_used=%llu last_pool=%llx\n",
            n, (unsigned long long)(uintptr_t)g_last_base, (unsigned long long)g_last_used,
            (unsigned long long)(uintptr_t)g_last_pool);
    for (int i = 0; i < lim; i++) {
        uint64_t a = h[i];
        fprintf(f, "h%d=%llx", i, (unsigned long long)a);
        for (int k = 0; k < maxw; k++) {
            uint64_t v = 0;
            struct iovec liov = { &v, 8 }, riov = { (void *)(uintptr_t)(a + k * 8), 8 };
            if (process_vm_readv(getpid(), &liov, 1, &riov, 1, 0) == 8) {
                outbuf[i * maxw + k] = v;
                fprintf(f, " %016llx", (unsigned long long)v);
            } else {
                fprintf(f, " ??");
            }
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

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
    dmNvnRegister("nvnCommandBufferSetViewport", (nvn_func_t)op_setviewport);
    dmNvnRegister("nvnCommandBufferSetScissor", (nvn_func_t)op_setviewport);
    dmNvnRegister("nvnCommandBufferDrawArrays", (nvn_func_t)op_draw3);
    dmNvnRegister("nvnCommandBufferDrawElements", (nvn_func_t)op_draw4);
    dmNvnRegister("nvnCommandBufferDrawElementsInstanced", (nvn_func_t)op_draw4);
    dmNvnRegister("nvnCommandBufferDispatchCompute", (nvn_func_t)op_dispatch);
    dmNvnRegister("nvnCommandBufferBindProgram", (nvn_func_t)op_bind_prog);
    dmNvnRegister("nvnCommandBufferBindVertexBuffers", (nvn_func_t)op_bind3);
    dmNvnRegister("nvnCommandBufferBindVertexAttribState", (nvn_func_t)op_bind3);
    dmNvnRegister("nvnCommandBufferBindUniformBuffer", (nvn_func_t)op_bind3);
    dmNvnRegister("nvnCommandBufferBindSeparateTextures", (nvn_func_t)op_bind3);
    dmNvnRegister("nvnCommandBufferCallCommands", (nvn_func_t)op_draw4);
    dmNvnRegister("nvnCommandBufferCopyBufferToTexture", (nvn_func_t)op_copy);

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
    dmNvnRegister("nvnTextureInitialize", (nvn_func_t)nvn_texture_init2);
    dmNvnRegister("nvnTextureGetStorageSize", (nvn_func_t)nvn_texture_get_storage_size);
    dmNvnRegister("nvnTextureGetViewOffset", (nvn_func_t)nvn_texture_get_view_offset);
    dmNvnRegister("nvnBufferInitialize", (nvn_func_t)nvn_buffer_init);
    dmNvnRegister("nvnBufferGetSize", (nvn_func_t)nvn_buffer_get_size);
    dmNvnRegister("nvnProgramSetShaders", (nvn_func_t)nvn_program_setshaders);
    dmNvnRegister("nvnSamplerInitialize", (nvn_func_t)nvn_sampler_init);
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
    dmNvnRegister("nvnTextureBuilderSetSize1D", (nvn_func_t)nvn_texb_set1d);
    dmNvnRegister("nvnTextureBuilderSetSize2D", (nvn_func_t)nvn_texb_set2d);
    dmNvnRegister("nvnTextureBuilderSetSize3D", (nvn_func_t)nvn_texb_set3d);
    dmNvnRegister("nvnTextureBuilderSetSwizzle", (nvn_func_t)nvn_texb_setswizzle);
    dmNvnRegister("nvnTextureBuilderSetDepthStencilMode", (nvn_func_t)nvn_texb_setdsmode);
    dmNvnRegister("nvnTextureBuilderSetStride", (nvn_func_t)nvn_texb_setstride);
    dmNvnRegister("nvnTextureBuilderSetStorage", (nvn_func_t)nvn_texb_setstorage);
    dmNvnRegister("nvnTextureBuilderSetPackagedTextureData", (nvn_func_t)nvn_texb_setpkgdata);
    dmNvnRegister("nvnTextureBuilderSetPackagedTextureLayout", (nvn_func_t)nvn_texb_setpkglayout);
    dmNvnRegister("nvnTextureBuilderSetGLTextureName", (nvn_func_t)nvn_texb_setgln);
    dmNvnRegister("nvnTextureBuilderGetTarget", (nvn_func_t)nvn_texb_gettarget);
    dmNvnRegister("nvnTextureBuilderGetWidth", (nvn_func_t)nvn_texb_getwidth);
    dmNvnRegister("nvnTextureBuilderGetHeight", (nvn_func_t)nvn_texb_getheight);
    dmNvnRegister("nvnTextureBuilderGetDepth", (nvn_func_t)nvn_texb_getdepth);
    dmNvnRegister("nvnTextureBuilderGetLevels", (nvn_func_t)nvn_texb_getlevels);
    dmNvnRegister("nvnTextureBuilderGetFormat", (nvn_func_t)nvn_texb_getformat);
    dmNvnRegister("nvnTextureBuilderGetSamples", (nvn_func_t)nvn_texb_getsamples);
    dmNvnRegister("nvnTextureBuilderGetStorageClass", (nvn_func_t)nvn_texb_getstoreclass);
    dmNvnRegister("nvnTextureBuilderGetDepthStencilMode", (nvn_func_t)nvn_texb_getdsmode);
    dmNvnRegister("nvnTextureBuilderGetSwizzle", (nvn_func_t)nvn_texb_getswizzle);
    dmNvnRegister("nvnTextureBuilderGetPackagedTextureData", (nvn_func_t)nvn_texb_getpkgdata);
    dmNvnRegister("nvnTextureBuilderGetPackagedTextureLayout", (nvn_func_t)nvn_texb_getpkglayout);
    dmNvnRegister("nvnTextureBuilderGetStride", (nvn_func_t)nvn_texb_getstride);
    dmNvnRegister("nvnTextureBuilderGetGLTextureName", (nvn_func_t)nvn_texb_getglname);
    dmNvnRegister("nvnTextureBuilderGetZCullStorageSize", (nvn_func_t)nvn_texb_getzull);
    dmNvnRegister("nvnTextureBuilderGetMemoryPool", (nvn_func_t)nvn_texb_getpool);
    dmNvnRegister("nvnTextureBuilderGetMemoryOffset", (nvn_func_t)nvn_texb_getoff);
    dmNvnRegister("nvnTextureBuilderGetRawStorageClass", (nvn_func_t)nvn_texb_getrawclass);

    dmNvnRegister("nvnBufferBuilderSetDefaults", (nvn_func_t)nvn_bufb_defaults);
    dmNvnRegister("nvnBufferBuilderSetDevice", (nvn_func_t)nvn_bufb_setdev);
    dmNvnRegister("nvnBufferBuilderSetSize", (nvn_func_t)nvn_bufb_setsize);
    dmNvnRegister("nvnBufferBuilderSetStorage", (nvn_func_t)nvn_bufb_setstorage);
    dmNvnRegister("nvnBufferBuilderGetSize", (nvn_func_t)nvn_bufb_getsize);
    dmNvnRegister("nvnBufferBuilderGetMemoryPool", (nvn_func_t)nvn_bufb_getpool);
    dmNvnRegister("nvnBufferBuilderGetMemoryOffset", (nvn_func_t)nvn_bufb_getoff);

    {
        pthread_t th;
        if (pthread_create(&th, NULL, nvn_stats_thread, NULL) == 0) {
            pthread_detach(th);
        }
    }
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
















