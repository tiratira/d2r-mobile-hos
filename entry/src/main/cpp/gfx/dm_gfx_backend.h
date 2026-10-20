/*
 * dm_gfx_backend.h — 图形后端接口。
 *
 * 上游架构:游戏只通过 nvnBootstrapLoader 拿 NVN(Switch 图形 API)
 * 函数表,gfx/nvn/hle_nvn.c 把调用录成命令,gfx/nvn/nvn_mtl.m 把
 * 命令回放为 Metal;着色器由离线翻译链(Maxwell -> SPIR-V(Ryujinx)
 * -> MSL)预翻译成 Metal 库。
 *
 * HOS 移植:NVN 接口层为纯 C 可平移;回放后端由 Metal 换成 Vulkan
 * (OHOS 提供 libvulkan)。着色器链可直接停在 SPIR-V,Vulkan 消费,
 * 省去上游的 SPIR-V -> MSL 一步。当前仓库内为接口骨架 + 探针,
 * 回放实现见 docs/PORTING.md 的图形层路线。
 */
#ifndef DM_GFX_BACKEND_H
#define DM_GFX_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DM_GFX_BACKEND_EGL_GLES = 0, /* 表面引导/演示路径(已实现) */
    DM_GFX_BACKEND_VULKAN   = 1  /* NVN 回放目标(未实现,见 PORTING.md) */
} DmGfxBackendKind;

const char *dmGfxBackendName(DmGfxBackendKind kind);

/* 编译期确认本 NDK 的 Vulkan 头可用;运行时探针说明后端状态。 */
const char *dmGfxVulkanProbe(void);

#ifdef __cplusplus
}
#endif
#endif /* DM_GFX_BACKEND_H */
