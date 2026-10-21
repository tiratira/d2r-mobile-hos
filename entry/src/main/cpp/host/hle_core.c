/*
 * hle_core.c — 门分派 + 每线程环境 + 未实现导入记名桩 + 导入解析。
 *
 * 上游 dm/host/hle.c 的 OHOS 移植,并接管 patch_nso_hos.py 新增的门类:
 *   tpidr_r / tpidr_w / tpidrro_r / param —— Apple 版以 tpidrro(TSD 基址)
 *   内联取槽;HOS 版这些访问全部走门,宿主侧用 pthread TSD,无内联槽。
 * 键号序与 tools/patch_nso_hos.py 的 KEY_INDEX 一致:
 *   0 K18 1 KTLS 2 KTLR 3 KGATE 4 KRES0 5 KRES1 6 KASPECT 7 KASPECTUI
 */
#include "host/dm.h"
#include "hal/dm_platform.h"
#include "loader/dm_guest_loader.h"

#ifdef __aarch64__
/* 客体线程栈很小(十几 KiB),svc/supervisor 处理里做文件/线程/malloc 等宿主
 * 调用会栈溢出。整段 gateway 处理切到 256 KiB 宿主栈(按嵌套深度偏移)。 */
uint64_t svc_dispatch(uint32_t num, uint64_t *regs, uint64_t orig_addr);
uint64_t dm_host_stack_run(uint64_t sp_top, uint64_t fn, uint64_t a, uint64_t b);
static _Thread_local uint8_t g_gw_stack[262144] __attribute__((aligned(16)));
static _Thread_local int g_gw_depth;
struct gw_call { uint32_t num; uint64_t *regs; uint64_t orig; uint64_t ret; };
static uint64_t gw_body(uint64_t arg)
{
    struct gw_call *c = (struct gw_call *)(uintptr_t)arg;
    c->ret = svc_dispatch(c->num, c->regs, c->orig);
    return c->ret;
}
static uint64_t gw_host_call(uint32_t num, uint64_t *regs, uint64_t orig)
{
    struct gw_call c = { num, regs, orig, 0 };
    int depth = g_gw_depth++;
    uint8_t *top = g_gw_stack + sizeof(g_gw_stack) - (size_t)depth * 65536;
    dm_host_stack_run((uint64_t)(uintptr_t)top, (uint64_t)(uintptr_t)&gw_body,
                      (uint64_t)(uintptr_t)&c, 0);
    g_gw_depth = depth;
    return c.ret;
}
#endif

/* ---------- 未实现导入:记名桩(entry_hos.S 静态桩表) ---------- */
#define DM_MAX_STUB 4096
static unsigned g_nstub;
static const char *g_stub_name[DM_MAX_STUB];
static uint8_t g_stub_seen[DM_MAX_STUB];
extern uint8_t unimpl_stubs[];      /* 每桩 8 字节(movz + b) */

uint64_t unimpl_c(unsigned idx, uint64_t *regs)
{
    if (idx < DM_MAX_STUB && !g_stub_seen[idx]++) {
        DMLOG_W("UNIMPL %{public}s (x0=%{public}llx x1=%{public}llx)",
                g_stub_name[idx], (unsigned long long)regs[0],
                (unsigned long long)regs[1]);
    }
    return 0;
}

/* ---------- HLE 登记段(ELF 自动 __start_/__stop_ 边界,lld 支持;类型与宏见 dm.h) ---------- */
extern const hle_ent __start_swhle[];
extern const hle_ent __stop_swhle[];
extern const hle_force_ent __start_swforce[];
extern const hle_force_ent __stop_swforce[];

/* 占位条目:ELF 只在段非空时才合成 __start_/__stop_ 边界符号 */
static void dm_section_placeholder(void) {}
__attribute__((used, section("swhle"), aligned(8)))
static const hle_ent dm_swhle_ph = {"__dm_placeholder__", (void *)dm_section_placeholder};
__attribute__((used, section("swforce"), aligned(8)))
static const hle_force_ent dm_swforce_ph = {"__dm_placeholder__", (void *)dm_section_placeholder,
                                            "段占位,保证边界符号存在"};

uint64_t hle_lookup(const char *name)
{
    for (const hle_ent *e = __start_swhle; e < __stop_swhle; e++) {
        if (strcmp(e->name, name) == 0) {
            return (uint64_t)e->fn;
        }
    }
    return 0;
}

int hle_registered_counts(uint32_t *hle, uint32_t *force)
{
    if (hle) {
        *hle = (uint32_t)(__stop_swhle - __start_swhle);
    }
    if (force) {
        *force = (uint32_t)(__stop_swforce - __start_swforce);
    }
    return 0;
}

uint64_t resolve_import(const char *name, int weak)
{
    uint64_t h = hle_lookup(name);
    if (h) {
        return h;
    }
    if (weak) {
        return 0;   /* 未解析弱导入保持零 */
    }
    for (unsigned k = 0; k < g_nstub; k++) {
        if (strcmp(g_stub_name[k], name) == 0) {
            return (uint64_t)&unimpl_stubs[k * 8];
        }
    }
    if (g_nstub >= DM_MAX_STUB) {
        die("stub overflow");
    }
    unsigned k = g_nstub++;
    g_stub_name[k] = name;
    return (uint64_t)&unimpl_stubs[k * 8];
}

void hle_force_report(void)
{
    for (const hle_force_ent *e = __start_swforce; e < __stop_swforce; e++) {
        DMLOG_I("HLE_FORCE %{public}s: %{public}s", e->name, e->why);
    }
}

/* ---------- 每线程环境(上游 thread.c 的 OHOS 版) ---------- */
#define TLS_SIZE 8192
#define TLR_SIZE 0x200
#define THREAD_OBJ_SIZE 0x400

static pthread_key_t g_keytls, g_keytlr, g_keyown;
static pthread_key_t g_param_keys[8];          /* 按 KEY_INDEX 序 */
static int g_keys_ready;
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static void own_free(void *p)
{
    void **own = (void **)p;
    free(own[0]);
    free(own[1]);
    free(own);
}

static void keys_create(void)
{
    if (pthread_key_create(&g_keytls, NULL) || pthread_key_create(&g_keytlr, NULL) ||
        pthread_key_create(&g_keyown, own_free)) {
        die("pthread_key_create failed");
    }
    for (int i = 0; i < 8; i++) {
        if (pthread_key_create(&g_param_keys[i], NULL)) {
            die("pthread_key_create(param) failed");
        }
    }
    g_keys_ready = 1;
}

void dm_guest_thread_init(void)
{
    pthread_once(&g_once, keys_create);
    /* 参数位种子:掌机/主机分辨率与 19:9 上限(前端可后续覆盖) */
    uint64_t res0 = front_game_res(0);
    uint64_t res1 = front_game_res(1);
    pthread_setspecific(g_param_keys[4], (void *)(uintptr_t)res0);   /* KRES0 */
    pthread_setspecific(g_param_keys[5], (void *)(uintptr_t)res1);   /* KRES1 */
    uint32_t bits;
    float aspect = 19.0f / 9.0f;
    memcpy(&bits, &aspect, 4);
    pthread_setspecific(g_param_keys[6], (void *)(uintptr_t)bits);   /* KASPECT */
    pthread_setspecific(g_param_keys[7], (void *)(uintptr_t)bits);   /* KASPECTUI */
    /* TLS 块 + TLR(含 SDK 临界区用的当前线程对象,TLR[0x1f8]) */
    void **own = calloc(2, sizeof *own);
    uint8_t *tlr = calloc(1, TLR_SIZE + THREAD_OBJ_SIZE);
    if (own == NULL || tlr == NULL) {
        die("thread env alloc failed");
    }
    own[0] = calloc(1, TLS_SIZE);
    own[1] = tlr;
    *(void **)(tlr + 0x1f8) = tlr + TLR_SIZE;
    pthread_setspecific(g_keytls, own[0]);
    pthread_setspecific(g_keytlr, tlr);
    pthread_setspecific(g_keyown, own);
}

uint64_t dm_guest_tls_get(void)
{
    return (uint64_t)(uintptr_t)pthread_getspecific(g_keytls);
}

void dm_guest_tls_set(uint64_t v)
{
    /* 客体自设线程指针:只改键槽,不动宿主 musl 的 tpidr_el0(libc 依赖它) */
    pthread_setspecific(g_keytls, (void *)(uintptr_t)v);
}

uint64_t dm_guest_tlr_get(void)
{
    return (uint64_t)(uintptr_t)pthread_getspecific(g_keytlr);
}

uint64_t dm_param_get(unsigned key_idx)
{
    if (!g_keys_ready || key_idx >= 8) {
        return 0;
    }
    return (uint64_t)(uintptr_t)pthread_getspecific(g_param_keys[key_idx]);
}

void dm_param_set(unsigned key_idx, uint64_t v)
{
    if (key_idx >= 8) {
        return;
    }
    pthread_once(&g_once, keys_create);
    pthread_setspecific(g_param_keys[key_idx], (void *)(uintptr_t)v);
}

/* ---------- 观测门回调(上游 gate_obs_set) ---------- */
#define OBS_MAX 32
static void (*g_obs_fn[OBS_MAX])(const uint64_t *regs);
static uint8_t g_obs_have[OBS_MAX];
void gate_obs_set(int id, void (*fn)(const uint64_t *regs))
{
    if (id <= 0 || id >= OBS_MAX) {
        die("观测门编号越界");
    }
    g_obs_fn[id] = fn;
}

/* ---------- 门表:按返回点(绝对地址)开地址哈希,一次命中 ---------- */
typedef struct { uint64_t ret; int kind; uint32_t arg; uint64_t orig; } gate_t;
#define GATE_BITS 12
static gate_t g_gate[1u << GATE_BITS];
static int g_ngate;
static inline unsigned gate_h(uint64_t a)
{
    return (unsigned)(((a >> 2) * 0x9E3779B97F4A7C15ull) >> (64 - GATE_BITS));
}

void gate_register_abs(uint64_t ret_abs, int kind, uint32_t arg, uint64_t orig_abs)
{
    if (kind == DM_GATE_OBS && (arg <= 0 || arg >= OBS_MAX)) {
        die("观测门编号越界");
    }
    if (++g_ngate > (1 << GATE_BITS) / 4) {
        die("gate overflow");
    }
    unsigned i = gate_h(ret_abs);
    while (g_gate[i].ret) {
        if (g_gate[i].ret == ret_abs) {
            die("门重复登记");
        }
        i = (i + 1) & ((1u << GATE_BITS) - 1);
    }
    g_gate[i] = (gate_t){ret_abs, kind, arg, orig_abs};
}

uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs)
{
    for (unsigned i = gate_h(ret_addr);; i = (i + 1) & ((1u << GATE_BITS) - 1)) {
        const gate_t *g = &g_gate[i];
        if (g->ret == ret_addr) {
            switch (g->kind) {
            case DM_GATE_CNTPCT:
            case DM_GATE_CNTVCT: {
                uint64_t t = dmPlatCounterRead();   /* 19.2 MHz 语义 */
                if (g->arg < 31) {
                    regs[g->arg] = t;
                }
                return t;
            }
            case DM_GATE_TPIDR_R:
                if (g->arg < 31) {
                    regs[g->arg] = dm_guest_tls_get();
                }
                return 0;
            case DM_GATE_TPIDRRO_R:
                if (g->arg < 31) {
                    regs[g->arg] = dm_guest_tlr_get();
                }
                return 0;
            case DM_GATE_TPIDR_W:
                /* arg 为源寄存器:msr tpidr_el0, xN → 写本线程 TLS 槽 */
                if (g->arg < 31) {
                    dm_guest_tls_set(regs[g->arg]);
                }
                return 0;
            case DM_GATE_PARAM: {
                unsigned key = g->arg >> 5, r = g->arg & 31;
                if (r < 31) {
                    regs[r] = dm_param_get(key);
                }
                return 0;
            }
            case DM_GATE_OBS:
                if (g->arg < OBS_MAX && g_obs_fn[g->arg]) {
                    g_obs_fn[g->arg](regs);
                }
                return 0;
            default:
#ifdef __aarch64__
                return gw_host_call(g->arg, regs, g->orig);
#else
                return svc_dispatch(g->arg, regs, g->orig);
#endif
            }
        }
        if (!g->ret) {
            DMLOG_E("unknown gate at %{public}llx", (unsigned long long)ret_addr);
            die("unknown gate");
        }
    }
}

uint32_t dm_gate_count(void)
{
    return (uint32_t)g_ngate;
}

/* ---------- 模块注册表(host_boot 登记加载好的 guest 模块;门表 orig 查询、
 * 剪口转调、Break 定位、image_bytes 记账共用) ---------- */
#define DM_MAX_MOD 8
static dm_module_t g_mods[DM_MAX_MOD];
static int g_nmods;

void dm_module_register(const char *name, const uint8_t *base, uint64_t size)
{
    if (g_nmods >= DM_MAX_MOD) {
        die("模块注册表满");
    }
    dm_module_t *m = &g_mods[g_nmods++];
    memset(m, 0, sizeof *m);
    snprintf(m->name, sizeof m->name, "%s", name);
    m->base = (uint8_t *)(uintptr_t)base;   /* 镜像在 .so 节内,装载器会原地写数据窗口 */
    m->size = size;
}

dm_module_t *dm_module_find(const char *name)
{
    for (int i = 0; i < g_nmods; i++) {
        if (strcmp(g_mods[i].name, name) == 0) {
            return &g_mods[i];
        }
    }
    return NULL;
}

const void *dm_swforce_lookup(const char *name)
{
    for (const hle_force_ent *e = __start_swforce; e < __stop_swforce; e++) {
        if (strcmp(e->name, name) == 0) {
            return e->fn;
        }
    }
    return NULL;
}

const dm_module_t *dm_module_of(uint64_t addr)
{
    for (int i = 0; i < g_nmods; i++) {
        if (addr >= (uint64_t)(uintptr_t)g_mods[i].base &&
            addr < (uint64_t)(uintptr_t)g_mods[i].base + g_mods[i].size) {
            return &g_mods[i];
        }
    }
    return NULL;
}

const dm_module_t *dm_module_iter(const dm_module_t *prev)
{
    if (!prev) {
        return g_nmods ? &g_mods[0] : NULL;
    }
    int idx = (int)(prev - g_mods);
    return idx + 1 < g_nmods ? &g_mods[idx + 1] : NULL;
}

int dm_max_modules(void)
{
    return g_nmods;
}

dm_module_t *dm_module_at(int idx)
{
    return idx >= 0 && idx < g_nmods ? &g_mods[idx] : NULL;
}
