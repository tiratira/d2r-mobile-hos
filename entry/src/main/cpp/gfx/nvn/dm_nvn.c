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

void dmNvnInit(void)
{
    dmNvnRegister("nvnDeviceGetProcAddress", (nvn_func_t)h_nvn_device_get_proc);
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
