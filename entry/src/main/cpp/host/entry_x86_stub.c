/*
 * entry_x86_stub.c — x86_64(PC 模拟器)构建的 ARM64 汇编替身。
 *
 * guest 重写镜像是 ARM64 代码,只能在 ARM64 真机原生执行;x86_64 模拟器
 * 构建提供同名符号,宿主框架(HAL/输入/EGL/Vulkan/NVN/门表)照常工作,
 * guest 执行路径(host_boot 已按 __aarch64__ 关闭)明确报告不可用。
 */
#include "host/dm.h"
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"

/* entry_hos.S 的记名桩表替身:4096 桩 × 8 字节(不执行,只占地址)。 */
uint8_t unimpl_stubs[4096 * 8];

uint64_t dm_gate(uint64_t ret_addr, uint64_t *regs)
{
    (void)ret_addr;
    (void)regs;
    OH_LOG_WARN(LOG_APP, "dm_gate: ARM64 guest execution unavailable on x86_64");
    return 0;
}

void dm_thread_enter(uint64_t entry, uint64_t arg, uint64_t sp)
{
    (void)sp;
    /* x86_64 上没有 ARM64 guest 代码可执行;唯一调用方是 svc HLE 自检的
     * 测试线程(entry 为宿主函数地址),按宿主约定直接调用。 */
    ((void (*)(uint64_t))entry)(arg);
}

/* 占位 guest 容器在 dm_guest_placeholder_x86.S(data-only,v3 布局,
 * 与 arm64 版同字节;在 x86 上永不执行,仅作加载器自检数据)。 */