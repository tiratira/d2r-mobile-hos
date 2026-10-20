/*
 * hle_svc.c — Horizon 内核系统调用分派(HOS 骨架)。
 *
 * 上游 dm/host/hle_svc.c 实现内存类调用(为客体保留整块地址空间并在其中
 * 切堆/栈),hle_thread.c 实现线程与同步调用。本文件先把分派框架、客体
 * 地址空间保留和日志打通;具体调号随真实三模块(你自己的游戏文件重写产
 * 物)接入后逐个补齐 —— 每个未实现调号只记一次日志并返回 0,便于按
 * hilog 清单排期。
 *
 * 宿主私有调号(0x80 起,上游 hle.h 登记表)统一走 svc_host_call 占位,
 * 它们多与游戏特性代码(dm_*.c)成对,留到特性平移阶段。
 */
#include "host/dm.h"
#include <sys/mman.h>

/* 客体地址空间窗口:真实接入时从这里切堆/栈(上游做法)。 */
#define DM_GUEST_VA_SIZE (4ull << 30)   /* 4 GiB 保留,不提交 */
static void *g_guest_va;

int dm_guest_va_reserve(void)
{
    if (g_guest_va) {
        return 0;
    }
    void *p = mmap(NULL, DM_GUEST_VA_SIZE, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        DMLOG_E("guest VA reserve failed");
        return -1;
    }
    g_guest_va = p;
    DMLOG_I("guest VA window @ %{public}p (%{public}llu MiB)", p,
            (unsigned long long)(DM_GUEST_VA_SIZE >> 20));
    return 0;
}

void *dm_guest_va_base(void)
{
    return g_guest_va;
}

static uint8_t g_svc_seen[256];

static uint64_t svc_host_call(uint32_t num, uint64_t *regs, uint64_t orig)
{
    /* 上游 hle.h 的 SVC_HOST_* 登记表(0x80-0x8e)在此分派到特性实现。 */
    (void)regs;
    (void)orig;
    if (!g_svc_seen[num & 0xff]++) {
        DMLOG_W("host svc %{public}#x not wired yet", num);
    }
    return 0;
}

uint64_t svc_dispatch(uint32_t num, uint64_t *regs, uint64_t orig_addr)
{
    if (num >= 0x80) {
        return svc_host_call(num, regs, orig_addr);
    }
    if (!g_svc_seen[num & 0xff]++) {
        DMLOG_W("UNIMPL svc %{public}u (x0=%{public}llx x1=%{public}llx orig=%{public}llx)",
                num, (unsigned long long)regs[0], (unsigned long long)regs[1],
                (unsigned long long)orig_addr);
    }
    return 0;
}
