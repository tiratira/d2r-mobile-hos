/*
 * dm_napi.cpp — NAPI 桥接 + XComponent 生命周期/触摸回调。
 *
 * 对应上游 dm/ios/Sources/front_ios.m(前端接口 host/front.h 的
 * iOS 实现):窗口/表面、输入注入、宿主信息。HOS 侧由 ArkUI 的
 * XComponent(libraryname="dmhost")挂接本模块。
 */
#include "napi/native_api.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_window/external_window.h>
#include <hilog/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal/dm_platform.h"
#include "input/dm_input.h"
#include "loader/dm_guest_loader.h"
#include "gfx/dm_gfx_backend.h"
#include "gfx/dm_vk.h"
#include "gfx/nvn/dm_nvn.h"
#include "gfx/nvn/dm_replay.h"
#include "render/dm_renderer.h"
#include "host/dm.h"

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static DmRenderer g_renderer;
static OHNativeWindow *g_window = nullptr;   /* SurfaceCreated 保存,后端切换用 */

static void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    if (window == nullptr) {
        DMLOG_E("surface created without window");
        return;
    }
    DMLOG_I("surface created");
    g_window = reinterpret_cast<OHNativeWindow *>(window);
    dmInputInit();
    dmPlatInit();
    /* 优先 Vulkan:EGL 在部分真机拿不到匹配 config,且事后从 UI 线程
     * Stop EGL 再 Start Vulkan 会和 EGL 渲染线程并发销毁上下文(实测崩在
     * libEGL/libGLES)。这里直接在 surface 回调里起 Vulkan,EGL 仅兜底。 */
    if (dmVkStart(g_window) != 0) {
        DMLOG_E("vk start failed, fallback to EGL");
        g_renderer.Start(g_window);
    }
}

static void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    (void)window;
}

static void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window)
{
    (void)window;
    (void)component;
    DMLOG_I("surface destroyed");
    dmVkStop();
    g_renderer.Stop();
    g_window = nullptr;
}

static void DispatchTouchEventCB(OH_NativeXComponent *component, void *window)
{
    OH_NativeXComponent_TouchEvent ev;
    if (OH_NativeXComponent_GetTouchEvent(component, window, &ev) != 0) {
        return;
    }
    for (int i = 0; i < ev.numPoints; i++) {
        const OH_NativeXComponent_TouchPoint &p = ev.touchPoints[i];
        DmTouchPhase phase;
        switch (p.type) {
            case OH_NATIVEXCOMPONENT_DOWN: phase = DM_TOUCH_DOWN; break;
            case OH_NATIVEXCOMPONENT_MOVE:  phase = DM_TOUCH_MOVE; break;
            case OH_NATIVEXCOMPONENT_UP:    phase = DM_TOUCH_UP; break;
            default:                        phase = DM_TOUCH_CANCEL; break;
        }
        dmInputPushTouch(p.id, p.x, p.y, phase);
    }
}

static void RegisterCallbacks(napi_env env, napi_value exports)
{
    napi_value obj = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &obj) != napi_ok
        || obj == nullptr) {
        DMLOG_E("init w/o xcomponent object (import path), callbacks not registered");
        return;
    }
    OH_NativeXComponent *component = nullptr;
    if (napi_unwrap(env, obj, reinterpret_cast<void **>(&component)) != napi_ok
        || component == nullptr) {
        DMLOG_E("unwrap native xcomponent failed");
        return;
    }
    static OH_NativeXComponent_Callback cb;
    cb.OnSurfaceCreated = OnSurfaceCreatedCB;
    cb.OnSurfaceChanged = OnSurfaceChangedCB;
    cb.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
    cb.DispatchTouchEvent = DispatchTouchEventCB;
    OH_NativeXComponent_RegisterCallback(component, &cb);
    DMLOG_I("xcomponent callbacks registered");
}

static const char *HostAbiName(void)
{
#ifdef __aarch64__
    return "arm64";
#else
    return "x86_64";
#endif
}

static napi_value GetHostInfo(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[512];
    char guest[160];
    dmGuestSelfTest(guest, sizeof(guest));
    uint64_t c = dmPlatCounterRead();
    snprintf(report, sizeof(report),
             "dmhost-hos %s | slots:tcb/tlr/gate | counter:%llu@19.2MHz | gfx:%s | %s",
             HostAbiName(), (unsigned long long)c,
             dmGfxBackendName(DM_GFX_BACKEND_EGL_GLES), guest);
    napi_value result;
    napi_create_string_utf8(env, report, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* 实际 ABI:x86_64 模拟器只能跑 data-only 框架层,ARM64 真机才执行游戏 */
static napi_value HostAbi(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
    napi_create_string_utf8(env, HostAbiName(), NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value GuestExecSupported(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
#ifdef __aarch64__
    napi_create_int32(env, 1, &result);
#else
    napi_create_int32(env, 0, &result);
#endif
    return result;
}

static napi_value SetFilesDir(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 1) {
        return nullptr;
    }
    char dir[1024];
    size_t len = 0;
    napi_get_value_string_utf8(env, argv[0], dir, sizeof(dir), &len);
    dmPlatSetFilesDir(dir);
    return nullptr;
}

static napi_value GuestSelfTest(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[160];
    dmGuestSelfTest(report, sizeof(report));
    napi_value result;
    napi_create_string_utf8(env, report, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value HostSelfTest(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[640];
    char modules[320];
    dmHostProbeModules(modules, sizeof(modules));
    char boot[160];
    int rc = dmHostBoot("main", boot, sizeof(boot));
    char svc[128];
    int svc_rc = dm_svc_selftest(svc, sizeof(svc));
    snprintf(report, sizeof(report), "%s | %s | %s%s", modules, boot, svc,
             svc_rc == 0 ? "" : "(FAIL)");
    napi_value result;
    napi_create_string_utf8(env, report, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value HostStart(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[1024];
    int rc = dmHostStart(report, sizeof(report));
    char out[1100];
    snprintf(out, sizeof(out), "%s%s", report, rc == 0 ? "" : " (FAIL)");
    napi_value result;
    napi_create_string_utf8(env, out, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value BootStage(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
    napi_create_string_utf8(env, dmHostBootStage(), NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value ServiceSelfTest(napi_env env, napi_callback_info info)
{
    (void)info;
    char fs[224], oe[128], hid[160], tmv[128], misc[128], acc[160];
    int r1 = dmFsSelfTest(fs, sizeof(fs));
    int r2 = dmOeSelfTest(oe, sizeof(oe));
    int r3 = dmHidSelfTest(hid, sizeof(hid));
    int r4 = dmTimeSelfTest(tmv, sizeof(tmv));
    int r5 = dmMiscSelfTest(misc, sizeof(misc));
    int r6 = dmAccountSelfTest(acc, sizeof(acc));
    char out[1100];
    snprintf(out, sizeof(out), "%s | %s | %s | %s | %s | %s%s", fs, oe, hid, tmv, misc, acc,
             (r1 || r2 || r3 || r4 || r5 || r6) ? " (FAIL)" : "");
    napi_value result;
    napi_create_string_utf8(env, out, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value AudioSelfTest(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[192];
    int rc = dmAudioSelfTest(report, sizeof(report));
    char out[220];
    snprintf(out, sizeof(out), "%s%s", report, rc == 0 ? "" : " (FAIL)");
    napi_value result;
    napi_create_string_utf8(env, out, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value PlayTone(napi_env env, napi_callback_info info)
{
    (void)info;
    int rc = dmAudioPlayTone(800);
    char out[160];
    snprintf(out, sizeof(out), "playTone(800ms) -> %d | %s", rc, dmAudioReport());
    napi_value result;
    napi_create_string_utf8(env, out, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value GfxProbe(napi_env env, napi_callback_info info)
{
    (void)info;
    char report[512];
    snprintf(report, sizeof(report), "%s | %s | %s",
             dmVkProbe(), dmGfxBackendName(DM_GFX_BACKEND_EGL_GLES), dmNvnSelfTest());
    napi_value result;
    napi_create_string_utf8(env, report, NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value SetSurfaceId(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    char sidbuf[32] = {0};
    size_t sidlen = 0;
    if (argc > 0) {
        napi_get_value_string_utf8(env, argv[0], sidbuf, sizeof(sidbuf), &sidlen);
    }
    uint64_t sid = strtoull(sidbuf, nullptr, 10);
    DMLOG_I("setSurfaceId called");
    OHNativeWindow *w = nullptr;
    int rc = OH_NativeWindow_CreateNativeWindowFromSurfaceId(sid, &w);
    if (rc != 0 || w == nullptr) {
        DMLOG_E("create native window from surface id failed");
    } else {
        g_window = w;
        int vk = dmVkStart(w);
        DMLOG_I("native window ok, vk start requested");
        rc = vk;
    }
    napi_value result;
    napi_create_int32(env, rc, &result);
    return result;
}

static napi_value UseVulkan(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
    int rc = -1;
    if (dmVkRunning()) {
        rc = 0;               /* 已在 surface 回调里起好,不要重复 Stop/Start */
    } else if (g_window != nullptr) {
        g_renderer.Stop();
        rc = dmVkStart(g_window);
    }
    napi_create_int32(env, rc, &result);
    return result;
}

static napi_value UseGles(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
    int rc = -1;
    if (g_window != nullptr) {
        dmVkStop();
        rc = g_renderer.Start(g_window);
    }
    napi_create_int32(env, rc, &result);
    return result;
}

static napi_value TouchInject(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value argv[4] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 4) {
        return nullptr;
    }
    int32_t id = 0, phase = 0;
    double x = 0, y = 0;
    napi_get_value_int32(env, argv[0], &id);
    napi_get_value_double(env, argv[1], &x);
    napi_get_value_double(env, argv[2], &y);
    napi_get_value_int32(env, argv[3], &phase);
    dmInputPushTouch(id, (float)x, (float)y, (DmTouchPhase)phase);
    return nullptr;
}

static napi_value SetStick(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 2) {
        return nullptr;
    }
    double dx = 0, dy = 0;
    napi_get_value_double(env, argv[0], &dx);
    napi_get_value_double(env, argv[1], &dy);
    dmInputSetStick((float)dx, (float)dy);
    return nullptr;
}

static napi_value SetPadButton(napi_env env,napi_callback_info info){size_t argc=2;napi_value argv[2]{};napi_get_cb_info(env,info,&argc,argv,nullptr,nullptr);if(argc==2){int32_t bit=0,pressed=0;napi_get_value_int32(env,argv[0],&bit);napi_get_value_int32(env,argv[1],&pressed);dmInputSetButton(bit,pressed);}return nullptr;}

static napi_value SetPadChord(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2]{};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    uint32_t mask = 0;
    int32_t pressed = 0;
    if (argc == 2 && napi_get_value_uint32(env, argv[0], &mask) == napi_ok &&
        napi_get_value_int32(env, argv[1], &pressed) == napi_ok)
        dmInputSetChord(mask, pressed);
    return nullptr;
}

static napi_value CastSkill(napi_env env, napi_callback_info info)
{
    size_t argc = 4;
    napi_value argv[4] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc < 4) {
        return nullptr;
    }
    int32_t slot = 0, hold = 0;
    double dx = 0, dy = 0;
    napi_get_value_int32(env, argv[0], &slot);
    napi_get_value_double(env, argv[1], &dx);
    napi_get_value_double(env, argv[2], &dy);
    napi_get_value_int32(env, argv[3], &hold);
    dmInputCastSkill(slot, (float)dx, (float)dy, hold);
    return nullptr;
}

static napi_value ArmCapture(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1]{};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    int frames = 20;
    if (argc >= 1) napi_get_value_int32(env, argv[0], &frames);
    dmReplayArmCapture(frames);
    napi_value undef{};
    return undef;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    DMLOG_I("napi Init called");
    napi_property_descriptor desc[] = {
        {"getHostInfo", nullptr, GetHostInfo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"armCapture", nullptr, ArmCapture, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hostAbi", nullptr, HostAbi, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"guestExecSupported", nullptr, GuestExecSupported, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setFilesDir", nullptr, SetFilesDir, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"guestSelfTest", nullptr, GuestSelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hostSelfTest", nullptr, HostSelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hostStart", nullptr, HostStart, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"bootStage", nullptr, BootStage, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"serviceSelfTest", nullptr, ServiceSelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"audioSelfTest", nullptr, AudioSelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"playTone", nullptr, PlayTone, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"gfxProbe", nullptr, GfxProbe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"useVulkan", nullptr, UseVulkan, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSurfaceId", nullptr, SetSurfaceId, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"useGles", nullptr, UseGles, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"touchInject", nullptr, TouchInject, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setPadButton", nullptr, SetPadButton, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setPadChord", nullptr, SetPadChord, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setStick", nullptr, SetStick, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"castSkill", nullptr, CastSkill, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    RegisterCallbacks(env, exports);
    dmCrashInstall();
    dmPlatInit();
    dmInputInit();
    return exports;
}
EXTERN_C_END

static napi_module dmhostModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "dmhost",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterDmhostModule(void)
{
    napi_module_register(&dmhostModule);
}

