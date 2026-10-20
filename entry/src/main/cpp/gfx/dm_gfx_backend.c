#include "dm_gfx_backend.h"

#include <vulkan/vulkan.h>

const char *dmGfxBackendName(DmGfxBackendKind kind)
{
    switch (kind) {
        case DM_GFX_BACKEND_EGL_GLES:
            return "egl-gles3";
        case DM_GFX_BACKEND_VULKAN:
            return "vulkan";
        default:
            return "unknown";
    }
}

const char *dmGfxVulkanProbe(void)
{
#ifdef VK_API_VERSION_1_0
    /* 头与 libvulkan.so 就绪;NVN->Vulkan 回放器未实现。
     * 实现要点见 docs/PORTING.md"图形层":
     *  1) gfx/nvn/hle_nvn.c 平移为 NVN 命令录制层;
     *  2) nvn_mtl.m 的 Metal 回放逐文件改写为 Vulkan;
     *  3) 着色器链止步 SPIR-V(Ryujinx Spv.Generator 产物)。
     */
    return "vulkan: headers+lib present, NVN replay not implemented";
#else
    return "vulkan: headers unavailable";
#endif
}
