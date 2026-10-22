#define _GNU_SOURCE 1
#include <sys/stat.h>
#include <sys/uio.h>
/* NVN bootstrap, typed runtime registration, fallback tracing and shader keys. */
#include "gfx/nvn/dm_nvn.h"
#include "gfx/tex/nvn_bcdec.h"
#include "gfx/tex/nvn_deswizzle.h"
#include "gfx/tex/nvn_tex.h"
#include "host/dm.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define NVN_TABLE_MAX 1024

typedef struct {
    char name[64];
    uint8_t called;
} nvn_slot;

static nvn_slot g_table[NVN_TABLE_MAX];
static nvn_func_t g_fns[NVN_TABLE_MAX];
static uint32_t g_count;
static uint32_t g_called;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

/* 占位返回用的非空缓冲:让未实现的 NVN 入口返回非 0,避免引擎把空指针当对象/函数表用 */
uint8_t dm_nvn_dummy[0x100];

/* 未知 NVN 入口的统一占位函数:返回 0(NVN 句柄/结果),不跳空 */
static uint64_t nvn_unimpl_fn(void) { return 0; }

/* nvnDeviceGetProcAddress(device, name):游戏/引擎用它取 NVN 设备函数表。
 * 返回宿主已实现的真实函数;没有实现的回统一占位函数(非空,避免引擎把
 * 0 存进函数表后跳飞)。每个名字首次解析时打一行 hilog —— 这就是接下来
 * 要逐个实现的 NVN 录制入口清单。 */
/* 解析出的「宿主函数地址 -> NVN 名字」表:崩溃现场从游戏 GOT 槽读到函数
 * 指针后可以反查是哪个 NVN 入口(引擎在初始化时把两者写进自己的数据段)。 */
#define NVN_RESOLVED_MAX 640
static struct {
    uint64_t addr;
    char name[64];
} g_resolved[NVN_RESOLVED_MAX];
static int g_resolved_n;
static pthread_mutex_t g_resmu = PTHREAD_MUTEX_INITIALIZER;

static void nvn_record_resolved(uint64_t addr, const char *name) {
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

const char *dmNvnNameForAddr(uint64_t addr) {
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

static uint64_t h_nvn_device_get_proc(void *dev, const char *name) {
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

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
#define ROR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
static void sha256_hex16(const void *in, size_t len, char out[17]) {
    const uint8_t *p = (const uint8_t *)in;
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint32_t w[64];
    size_t i = 0;
    while (i + 64 <= len) {
        for (int k = 0; k < 16; k++)
            w[k] = ((uint32_t)p[i + k * 4] << 24) | ((uint32_t)p[i + k * 4 + 1] << 16) |
                   ((uint32_t)p[i + k * 4 + 2] << 8) | p[i + k * 4 + 3];
        i += 64;
        for (int k = 16; k < 64; k++) {
            uint32_t s0 = ROR32(w[k - 15], 7) ^ ROR32(w[k - 15], 18) ^ (w[k - 15] >> 3);
            uint32_t s1 = ROR32(w[k - 2], 17) ^ ROR32(w[k - 2], 19) ^ (w[k - 2] >> 10);
            w[k] = w[k - 16] + s0 + w[k - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int k = 0; k < 64; k++) {
            uint32_t S1 = ROR32(e, 6) ^ ROR32(e, 11) ^ ROR32(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + K256[k] + w[k];
            uint32_t S0 = ROR32(a, 2) ^ ROR32(a, 13) ^ ROR32(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }
    /* 末尾补齐:0x80 + 0 + 64 位长度(恰好 64 字节时也要多一个块) */
    uint8_t pad[128];
    size_t rem = len - i;
    memset(pad, 0, sizeof(pad));
    if (rem)
        memcpy(pad, p + i, rem);
    pad[rem] = 0x80;
    size_t padlen = (rem < 56) ? 64 : 128;
    uint64_t bits = (uint64_t)len * 8;
    for (int k = 0; k < 8; k++)
        pad[padlen - 1 - k] = (uint8_t)(bits >> (8 * k));
    for (size_t off = 0; off < padlen; off += 64) {
        for (int k = 0; k < 16; k++)
            w[k] = ((uint32_t)pad[off + k * 4] << 24) | ((uint32_t)pad[off + k * 4 + 1] << 16) |
                   ((uint32_t)pad[off + k * 4 + 2] << 8) | pad[off + k * 4 + 3];
        for (int k = 16; k < 64; k++) {
            uint32_t s0 = ROR32(w[k - 15], 7) ^ ROR32(w[k - 15], 18) ^ (w[k - 15] >> 3);
            uint32_t s1 = ROR32(w[k - 2], 17) ^ ROR32(w[k - 2], 19) ^ (w[k - 2] >> 10);
            w[k] = w[k - 16] + s0 + w[k - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int k = 0; k < 64; k++) {
            uint32_t S1 = ROR32(e, 6) ^ ROR32(e, 11) ^ ROR32(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + K256[k] + w[k];
            uint32_t S0 = ROR32(a, 2) ^ ROR32(a, 13) ^ ROR32(a, 22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + S0 + mj;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }
    static const char hexd[] = "0123456789abcdef";
    for (int q = 0; q < 2; q++)
        for (int k = 0; k < 4; k++) {
            uint8_t byte = (uint8_t)(h[q] >> (24 - 8 * k));
            out[q * 8 + k * 2] = hexd[byte >> 4];
            out[q * 8 + k * 2 + 1] = hexd[byte & 15];
        }
    out[16] = 0;
}

#include "dm_nvn_trace.inc"
#include "dm_nvn_fallback_report.inc"
/* Capture diagnostics must survive the bounded device log. These counters only
 * belong to fallback wrappers; implemented draw entry points replace them. */
unsigned dmNvnFallbackDrawMask(void) {
    return (atomic_load(&nvn_trace_n_nvnCommandBufferDrawArraysInstanced) != 0) |
           ((atomic_load(&nvn_trace_n_nvnCommandBufferDrawElementsBaseVertex) != 0) << 1) |
           ((atomic_load(&nvn_trace_n_nvnCommandBufferDrawArraysIndirect) != 0) << 2) |
           ((atomic_load(&nvn_trace_n_nvnCommandBufferDrawElementsIndirect) != 0) << 3) |
           ((atomic_load(&nvn_trace_n_nvnCommandBufferMultiDrawArraysIndirectCount) != 0) << 4) |
           ((atomic_load(&nvn_trace_n_nvnCommandBufferMultiDrawElementsIndirectCount) != 0) << 5);
}
#include "nvn_obj.h"
extern void dmNvnInitRuntime(void);
void dmNvnShaderKey(const void *p, char out[17]) { sha256_hex16(p, 0x880, out); }
static pthread_once_t initialized = PTHREAD_ONCE_INIT;
static void init_runtime(void) {
    struct {
        char key[17];
        unsigned char guard[48];
    } result;
    unsigned char zero[0x880] = {0};
    memset(&result, 0xa5, sizeof result);
    dmNvnShaderKey(zero, result.key);
    if (strcmp(result.key, "5626d9a8f1d7364b"))
        die("NVN SHA-256 regression: incorrect key");
    for (unsigned i = 0; i < sizeof result.guard; i++)
        if (result.guard[i] != 0xa5)
            die("NVN SHA-256 regression: buffer overrun");
    dm_nvn_register_traces();
    dmNvnRegister("nvnDeviceGetProcAddress", (nvn_func_t)h_nvn_device_get_proc);
    dmNvnInitRuntime();
    DMLOG_I("NVN runtime initialized; SHA-256 key + guard PASS, entries=%{public}u", g_count);
}
void dmNvnInit(void) { pthread_once(&initialized, init_runtime); }
int dmNvnTextureLinearize(const void *tex, unsigned level, unsigned layer, const void *src, void *dst,
                          size_t pitch) {
    const tex_t *t = (const tex_t *)tex;
    return t ? nvn_deswizzle(&t->d, level, layer, src, dst, pitch) : -1;
}
#ifdef __aarch64__
extern uint8_t dm_nvn_thunks[];

uint64_t dm_nvn_unimpl_c(uint64_t idx, uint64_t *regs) {
    pthread_mutex_lock(&g_mu);
    if (idx < g_count && !g_table[idx].called) {
        g_table[idx].called = 1;
        g_called++;
        DMLOG_W("NVN %{public}s: first call (thunk, args x0=%{public}llx x1=%{public}llx x2=%{public}llx)",
                g_table[idx].name, (unsigned long long)regs[0], (unsigned long long)regs[1],
                (unsigned long long)regs[2]);
    }
    pthread_mutex_unlock(&g_mu);
    return 0;
}
#else
static uint64_t nvn_stub0(void) { return 0; }
#endif

void dmNvnRegister(const char *name, nvn_func_t fn) {
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

uint64_t dmNvnBootstrapLoader(const char *name) {
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
        return (uint64_t)(uintptr_t)g_fns[id]; /* 已实现/录制入口 */
    }
#ifdef __aarch64__
    {
        uint64_t r = (uint64_t)(uintptr_t)(dm_nvn_thunks + (size_t)id * 8);
        nvn_record_resolved(r, name);
        return r;
    }
#else
    return (uint64_t)(uintptr_t)&nvn_stub0; /* data-only:表/名字自检,不执行 */
#endif
}

/* 上游同一接管点:main 导入、sdk 定义,必须 force */
HLE_FORCE("nvnBootstrapLoader", "NVN 图形接口整块换成宿主(后续 Vulkan 回放),sdk 的 NVN 驱动不执行", void *,
          h_nvn_bootstrap, const char *name) {
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
HLE_FORCE("_ZN2nv18InitializeGraphicsEPvm", "NVN 驱动初始化:宿主不跑 sdk 图形驱动", uint32_t, h_nv_InitGfx,
          void *mem, size_t size) {
    (void)mem;
    (void)size;
    return 0;
}
HLE_FORCE("_ZN2nv16FinalizeGraphicsEv", "NVN 驱动收尾:空操作", void, h_nv_FinGfx, void) {}
HLE_FORCE("_ZN2nv22SetGraphicsServiceNameEPKc", "NVN 服务名:记录但不连服务", void, h_nv_SetName,
          const char *n) {
    (void)n;
}
HLE_FORCE("_ZN2nv26InitializeGraphicsDevtoolsEPvm", "NVN devtools:宿主不支持", uint32_t, h_nv_InitDev,
          void *mem, size_t size) {
    (void)mem;
    (void)size;
    return 0;
}
HLE_FORCE("_ZN2nv21GetGraphicsMemoryInfoEPNS_10MemoryInfoE", "NVN 显存统计:给 0", void, h_nv_MemInfo,
          void *info) {
    (void)info;
}

uint32_t dmNvnEntryCount(void) { return g_count; }

uint32_t dmNvnCalledCount(void) { return g_called; }

const char *dmNvnSelfTest(void) {
    static char report[192];
    /* 与上游一致的引导序列:先取构建器/设备/呈现入口 */
    dmNvnBootstrapLoader("nvnDeviceBuilderInitialize");
    dmNvnBootstrapLoader("nvnDeviceInitialize");
    dmNvnBootstrapLoader("nvnQueuePresent");
    uint64_t a = dmNvnBootstrapLoader("nvnDeviceInitialize");
    uint64_t b = dmNvnBootstrapLoader("nvnNoSuchEntry");
    snprintf(report, sizeof(report), "nvn: table=%u called=%u dispatch=%s unknown->%s", dmNvnEntryCount(),
             dmNvnCalledCount(), a ? "ok" : "null", b == 0 ? "null" : "non-null");
    return report;
}
