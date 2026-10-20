/*
 * host_boot.c — 宿主启动序(上游 dm/host/main.c 的 HOS 骨架)。
 *
 * dmHostProbeModules():经 loader_hos(宿主扮演 Horizon rtld)装载三个内嵌
 * 模块 —— 原地重定位(数据窗口 mprotect RW)、符号表解析、门表登记 —— 并出
 * 自检报告(每模块符号数/三类重定位计数 + SDK 启动序关键符号探测)。
 * x86_64 构建编 data-only 镜像:同一装载/重定位路径照跑,仅不执行。
 *
 * dmHostBoot():占位容器冒烟(线程环境 → 门分派 → 寄存器回写 → 返回)。
 * 真实 nnMain 启动序(_init_libc0 → nnosInitialize → … → nnMain)待
 * fs/oe/hid 服务层 HLE 就绪后接入(符号已可在自检报告里看到)。
 */
#include "host/dm.h"
#include "host/front_hos.h"
#include "hal/dm_platform.h"
#include "loader/dm_guest_loader.h"

#include <pthread.h>
#include <sys/mman.h>

/* 真实模块容器(hos_embed.py v3 产物;缺省弱符号为空,CMake 探测编入) */
extern const uint8_t __dm_guest_main_start[] __attribute__((weak));
extern const uint8_t __dm_guest_main_meta[] __attribute__((weak));
extern const uint8_t __dm_guest_sdk_start[] __attribute__((weak));
extern const uint8_t __dm_guest_sdk_meta[] __attribute__((weak));
extern const uint8_t __dm_guest_subsdk0_start[] __attribute__((weak));
extern const uint8_t __dm_guest_subsdk0_meta[] __attribute__((weak));

#ifdef __aarch64__
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
#endif

void dm_thread_returned(void)
{
    DMLOG_W("guest entry returned(Horizon 线程应经 ExitThread 退出)");
}

/*
 * 装载三个内嵌模块并统一重定位(上游 load_modules 的顺序:先全部登记解析,
 * 再统一重定位 —— 跨模块符号要先都在位)。幂等:重复调用只回报缓存结果。
 */
int dmHostProbeModules(char *report, int report_size)
{
    static int s_done;
    static char s_cache[448];
    if (s_done) {
        snprintf(report, report_size, "%s", s_cache);
        return 0;
    }
    s_done = 1;
    struct {
        const char *name;
        const uint8_t *image;
        const uint8_t *meta;
    } mods[3] = {
        {"main", __dm_guest_main_start, __dm_guest_main_meta},
        {"sdk", __dm_guest_sdk_start, __dm_guest_sdk_meta},
        {"subsdk0", __dm_guest_subsdk0_start, __dm_guest_subsdk0_meta},
    };
    int loaded = 0;
    int off = 0;
    for (int i = 0; i < 3; i++) {
        if (mods[i].image == NULL || mods[i].meta == NULL) {
            continue;   /* 未嵌入(弱符号空) */
        }
        if (dm_load_module(mods[i].image, mods[i].meta, mods[i].name) != 0) {
            off += snprintf(report + off, report_size - off, "%s:FAIL ", mods[i].name);
            continue;
        }
        loaded++;
    }
    if (loaded == 0) {
        snprintf(s_cache, sizeof(s_cache), "no embedded game modules (placeholder only)");
        snprintf(report, report_size, "%s", s_cache);
        return 0;
    }
    dm_relocate_all();
    char lr[192];
    dm_loader_report(lr, sizeof(lr));
    snprintf(s_cache, sizeof(s_cache), "%d modules | %s", loaded, lr);
    snprintf(report, report_size, "%s", s_cache);
    return 0;
}

int dmHostBoot(const char *which, char *report, int report_size)
{
    /* 占位容器自检:验证「线程环境 → 门分派 → 寄存器回写 → 返回」全链路;
     * 幂等防重复登记。 */
    static int s_done;
    if (s_done) {
        snprintf(report, report_size, "boot: already run (see first report)");
        return 0;
    }
    s_done = 1;
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
    /* 主线程登记(线程类 svc 要用),客体地址空间由首个 svc 触发预留 */
    dm_svc_thread_main_init();
#ifdef __aarch64__
    boot_ctx c;
    c.mod = m;
    c.entry = (uint64_t)(m.image + m.hdr.entry_off);
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
#else
    (void)m;
    snprintf(report, report_size,
             "boot: guest 执行仅限 ARM64 真机(x86_64 模拟器跑框架层)");
    return 0;
#endif
}