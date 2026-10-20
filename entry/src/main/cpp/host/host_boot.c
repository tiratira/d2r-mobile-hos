/*
 * host_boot.c — 宿主启动序(上游 dm/host/main.c 的 HOS 骨架)。
 *
 * dmHostBoot():加载内嵌 guest 容器 → 校验 → 按门表注册门(绝对地址 =
 * 镜像基址 + ret_off)→ 准备每线程环境 → 在新 pthread 上以 dm_thread_enter
 * 调客体入口。真实三模块接入后,这里扩展为上游的 SDK 启动序(init_array、
 * nnMain、跨模块重定位);当前占位容器的入口本身是一个 tpidr_r 门桩,
 * 自检即可验证「线程环境 → 门分派 → 寄存器回写 → 返回」全链路。
 */
#include "host/dm.h"
#include "host/front_hos.h"
#include "hal/dm_platform.h"
#include "loader/dm_guest_loader.h"

#include <pthread.h>
#include <sys/mman.h>

typedef struct {
    DmGuestModule mod;
    uint64_t entry;
    uint64_t sp;
    int result;
    uint64_t x0_after;
} boot_ctx;

static void *guest_main(void *arg)
{
    boot_ctx *c = (boot_ctx *)arg;
    dm_guest_thread_init();
    dm_thread_enter(c->entry, 0, c->sp);
    return NULL;   /* dm_thread_enter 不返回;入口返回会走 dm_thread_returned */
}

void dm_thread_returned(void)
{
    DMLOG_W("guest entry returned(Horizon 线程应经 ExitThread 退出)");
}

int dmHostBoot(const char *which, char *report, int report_size)
{
    /* 当前只有占位容器;真实模块在此按 main/sdk/subsdk0 依次加载。 */
    (void)which;
    DmGuestModule m;
    if (dmGuestLoadEmbedded(__dm_guest_placeholder_start, UINT64_MAX, "main", &m) != 0 ||
        dmGuestVerify(&m) != 0) {
        snprintf(report, report_size, "boot: guest load/verify FAILED");
        return -1;
    }
    /* 注册门表 */
    for (uint32_t i = 0; i < m.hdr.gate_count; i++) {
        DmGuestGate g;
        if (dmGuestGateAt(&m, i, &g) != 0) {
            snprintf(report, report_size, "boot: gate read FAILED");
            return -1;
        }
        gate_register_abs((uint64_t)(m.image + g.ret_off), (int)g.kind, g.arg,
                          (uint64_t)(m.image + g.orig_off));
    }
    /* 客体地址空间窗口(真实接入时切堆/栈) */
    dm_guest_va_reserve();
    /* 客体主线程:宿主栈仅用于 dm_thread_enter 垫层,随即切客体栈 */
    boot_ctx c;
    c.mod = m;
    c.entry = (uint64_t)(m.image + m.hdr.entry_offset);
    c.sp = (uint64_t)mmap(NULL, 1 << 20, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (c.sp == (uint64_t)MAP_FAILED) {
        snprintf(report, report_size, "boot: guest stack mmap FAILED");
        return -1;
    }
    c.sp += (1 << 20) - 256;   /* 顶端留红区 */

    pthread_t t;
    if (pthread_create(&t, NULL, guest_main, &c) != 0) {
        snprintf(report, report_size, "boot: guest thread create FAILED");
        return -1;
    }
    pthread_join(t, NULL);
    munmap((void *)(c.sp - ((1 << 20) - 256)), 1 << 20);
    snprintf(report, report_size,
             "boot: OK(gates=%u image=%lluB entry穿过门桩,tpidr_r→x0 已回写)",
             m.hdr.gate_count, (unsigned long long)m.hdr.image_size);
    dmGuestUnload(&m);
    return 0;
}
