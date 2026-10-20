/*
 * dm_vk.h — Vulkan 回放后端(对应上游 gfx/nvn/nvn_mtl.m 的 Metal 角色)。
 */
#ifndef DM_VK_H
#define DM_VK_H

#include <cstdint>
#include <native_window/external_window.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 探针:建/毁一个无表面实例,返回设备可数的可读报告。 */
const char *dmVkProbe(void);

/* 在 XComponent 的 NativeWindow 上跑 Vulkan 呈现循环(清屏色动画),
 * 证明 实例→VK_OHOS_surface→交换链→呈现 全链路;返回 0 成功。 */
int dmVkStart(OHNativeWindow *window);
void dmVkStop(void);
int dmVkRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* DM_VK_H */
