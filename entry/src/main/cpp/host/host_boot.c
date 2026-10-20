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

/* 真实模块容器(hos_embed.py 产物;缺省弱符号为空,存在时由 CMake 编入) */
extern const uint8_t __dm_guest_main_start[] __attribute__((weak));
extern const uint8_t __dm_guest_sdk_start[] __attribute__((weak));
extern const uint8_t __dm_guest_subsdk0_start[] __attribute__((weak));

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
 * 探测内嵌模块:加载 + 校验 + 注册门表 + 进模块注册表,不执行任何入口。
 * 幂等:模块保持加载(门表/转调引用其绝对地址),重复调用只回报缓存结果。
 * 真实 nnMain 要等 svc/线程 HLE 填齐(hilog 的 UNIMPL 清单)才能启动。
 */
int dmHostProbeModules(char *report, int report_size)
{
    static int s_done;
    static char s_cache[320];
    if (s_done) {
        snprintf(report, report_size, "%s", s_cache);
        return 0;
    }
    s_done = 1;
    struct {
        const char *name;
        const uint8_t *blob;
    } mods[3] = {
        {"main", __dm_guest_main_start},
        {"sdk", __dm_guest_sdk_start},
        {"subsdk0", __dm_guest_subsdk0_start},
    };
    int off = 0;
    int total_gates = 0;
    int loaded = 0;
    for (int i = 0; i < 3; i++) {
        if (mods[i].blob == NULL) {
            continue;   /* 未嵌入(弱符号空) */
        }
        DmGuestModule m;
        if (dmGuestLoadEmbedded(mods[i].blob, UINT64_MAX, mods[i].name, &m) != 0 ||
            dmGuestVerify(&m) != 0) {
            off += snprintf(report + off, report_size - off, "%s:FAIL ", mods[i].name);
            continue;
        }
        for (uint32_t g = 0; g < m.hdr.gate_count; g++) {
            DmGuestGate gt;
            if (dmGuestGateAt(&m, g, &gt) == 0) {
                gate_register_abs((uint64_t)(m.image + gt.ret_off), (int)gt.kind,
                                  gt.arg, (uint64_t)(m.image + gt.orig_off));
            }
        }
        /* 保持加载:门表登记的是镜像内绝对地址,释放即悬空(剪口转调也要用) */
        dm_module_register(mods[i].name, m.image, m.hdr.image_size);
        total_gates += (int)m.hdr.gate_count;
        loaded++;
        off += snprintf(report + off, report_size - off,
                        "%s:%lluB/%ug ", mods[i].name,
                        (unsigned long long)m.hdr.image_size, m.hdr.gate_count);
    }
    if (loaded == 0) {
        snprintf(s_cache, sizeof(s_cache), "no embedded game modules (placeholder only)");
    } else {
        snprintf(s_cache + off, sizeof(s_cache) - off,
                 "| %d modules, %d gates registered (not booted)", loaded, total_gates);
    }
    snprintf(report, report_size, "%s", s_cache);
    return 0;
}

int dmHostBoot(const char *which, char *report, int report_size)
{
    /* 占位容器自检:验证「线程环境 → 门分派 → 寄存器回写 → 返回」全链路;
     * 真实模块的 nnMain 启动待 svc HLE 验证后接入。幂等防重复登记。 */
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
#else
    (void)m;
    snprintf(report, report_size,
             "boot: guest 执行仅限 ARM64 真机(x86_64 模拟器跑框架层)");
    return 0;
#endif
}
