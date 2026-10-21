/*
 * hle_vi.c — nn::vi(显示层):宿主没有 vi 服务
 * (上游 dm/gfx/hle_vi.c 的 HOS 版)。
 *
 * 真机上 nnMain 会调 nn::vi::Initialize/OpenDefaultDisplay/CreateLayer/
 * GetNativeWindow;不接管就会自己走 hipc 连 "sm:" → 宿主无服务 → diag::Abort
 * (真机实测:进游戏 0.03 秒闪退)。宿主给静态对象占位,原生窗口句柄之后
 * 由 NVN 回放层接管。
 */
#include "host/dm.h"

typedef uint32_t Result;
#define RES_OK 0u

static struct { uint32_t magic; } g_display = {0x50534944};          /* "DISP" */
static struct { uint32_t magic; } g_layer = {0x5259414c};            /* "LAYR" */
static struct { uint32_t magic; void *layer; } g_native = {0x4e49574e, NULL}; /* "NWIN" */

HLE_FORCE("_ZN2nn2vi10InitializeEv", "显示服务初始化;宿主无 vi 服务",
          void, h_vi_Init, void) {}

HLE_FORCE("_ZN2nn2vi18OpenDefaultDisplayEPPNS0_7DisplayE", "开默认显示;宿主给静态对象",
          Result, h_vi_OpenDisp, void **out)
{
    if (out) {
        *out = &g_display;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2vi11CreateLayerEPPNS0_5LayerEPNS0_7DisplayE", "建图层;宿主给静态对象",
          Result, h_vi_CreateLayer, void **out, void *disp)
{
    (void)disp;
    if (out) {
        *out = &g_layer;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn2vi15GetNativeWindowEPPvPNS0_5LayerE", "原生窗口句柄,交给 NVN 窗口 builder;宿主给静态对象",
          Result, h_vi_GetNative, void **out, void *layer)
{
    (void)layer;
    if (out) {
        *out = &g_native;
    }
    return RES_OK;
}
