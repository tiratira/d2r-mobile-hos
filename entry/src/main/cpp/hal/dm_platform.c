/*
 * dm_platform.c — OHOS 平台抽象层实现。
 */
#include "dm_platform.h"

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static pthread_key_t g_slots[DM_SLOT_COUNT];
static int g_slots_ready = 0;
static char g_files_dir[1024] = {0};

int dmPlatInit(void)
{
    return dmPlatThreadSlotInit();
}

int dmPlatThreadSlotInit(void)
{
    if (g_slots_ready) {
        return 0;
    }
    for (int i = 0; i < DM_SLOT_COUNT; i++) {
        if (pthread_key_create(&g_slots[i], NULL) != 0) {
            DMLOG_E("pthread_key_create(slot %{public}d) failed", i);
            return -1;
        }
    }
    g_slots_ready = 1;
    DMLOG_I("thread slots ready (tcb/tlr/gate)");
    return 0;
}

void *dmPlatThreadSlotGet(int slot)
{
    if (slot < 0 || slot >= DM_SLOT_COUNT || !g_slots_ready) {
        return NULL;
    }
    return pthread_getspecific(g_slots[slot]);
}

void dmPlatThreadSlotSet(int slot, void *value)
{
    if (slot < 0 || slot >= DM_SLOT_COUNT || !g_slots_ready) {
        return;
    }
    pthread_setspecific(g_slots[slot], value);
}

uint64_t dmPlatCounterHz(void)
{
    return DM_COUNTER_HZ;
}

uint64_t dmPlatCounterRead(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    /* ns -> 19.2 MHz ticks: ns * 19200000 / 1e9 */
    return (uint64_t)(((__uint128_t)ns * DM_COUNTER_HZ) / 1000000000ull);
}

void dmPlatSetFilesDir(const char *dir)
{
    if (dir == NULL) {
        return;
    }
    strncpy(g_files_dir, dir, sizeof(g_files_dir) - 1);
    DMLOG_I("files dir set: %{public}s", g_files_dir);
}

const char *dmPlatGetFilesDir(void)
{
    return g_files_dir;
}
