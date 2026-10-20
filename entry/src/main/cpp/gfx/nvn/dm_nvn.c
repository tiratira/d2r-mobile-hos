/*
 * dm_nvn.c — NVN 引导接口骨架:函数表 + 记名桩 + 一次性日志。
 *
 * 上游 hle_nvn.c 开头同样是一张「NVN 入口 → 宿主实现」的表;录制器就位
 * 之前,每个入口都是记名桩(调一次记一行,返回 0),与上游 unimpl 桩同款
 * 策略 —— 真实镜像跑起来后按 hilog 清单排期实现录制/回放。
 */
#include "gfx/nvn/dm_nvn.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_W(...) OH_LOG_WARN(LOG_APP, __VA_ARGS__)

#define NVN_TABLE_MAX 512

typedef struct {
    char name[64];
    nvn_func_t fn;
    uint8_t called;
} nvn_slot;

static nvn_slot g_table[NVN_TABLE_MAX];
static uint32_t g_count;
static uint32_t g_called;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;

static void nvn_stub(void)
{
    /* 桩不知道自己的名字;调用计数经 dmNvnRegister 的包装实现。 */
}

/* 包装桩:进入时给所属槽记一次调用。用每槽一个的小跳板不可行(不许运行
 * 时生成代码),改为「表驱动入口 + 槽号放 x9」不可行 —— NVN ABI 是普通
 * C 调用。骨架阶段用统一桩 + 调用点日志(入口名由 bootstrap 表给出)。 */
static uint64_t nvn_dispatch_by_name(const char *name)
{
    pthread_mutex_lock(&g_mu);
    for (uint32_t i = 0; i < g_count; i++) {
        if (strcmp(g_table[i].name, name) == 0) {
            if (!g_table[i].called) {
                g_table[i].called = 1;
                g_called++;
                DMLOG_W("NVN %{public}s: first call (stub, recorder pending)", name);
            }
            pthread_mutex_unlock(&g_mu);
            return (uint64_t)(uintptr_t)nvn_stub;
        }
    }
    pthread_mutex_unlock(&g_mu);
    DMLOG_W("NVN %{public}s: unknown entry", name);
    return 0;
}

void dmNvnRegister(const char *name, nvn_func_t fn)
{
    pthread_mutex_lock(&g_mu);
    if (g_count < NVN_TABLE_MAX && name != NULL) {
        strncpy(g_table[g_count].name, name, sizeof(g_table[g_count].name) - 1);
        g_table[g_count].fn = fn ? fn : nvn_stub;
        g_count++;
    }
    pthread_mutex_unlock(&g_mu);
}

uint64_t dmNvnBootstrapLoader(const char *name)
{
    if (name == NULL) {
        return 0;
    }
    return nvn_dispatch_by_name(name);
}

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
    /* 与上游一致的引导序列:先取 DeviceBuilderInitialize 等
       构建器入口。骨架登记三枚示例入口验证表与分发。 */
    dmNvnRegister("nvnDeviceBuilderInitialize", NULL);
    dmNvnRegister("nvnDeviceInitialize", NULL);
    dmNvnRegister("nvnQueuePresent", NULL);
    uint64_t a = dmNvnBootstrapLoader("nvnDeviceInitialize");
    uint64_t b = dmNvnBootstrapLoader("nvnNoSuchEntry");
    snprintf(report, sizeof(report),
             "nvn: table=%u called=%u dispatch=%s unknown->%s",
             dmNvnEntryCount(), dmNvnCalledCount(),
             a ? "ok" : "null", b == 0 ? "null" : "non-null");
    return report;
}
