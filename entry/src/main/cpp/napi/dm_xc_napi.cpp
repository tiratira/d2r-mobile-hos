/*
 * dm_xc_napi.cpp — 独立的 XComponent napi 模块(libdmxe.so)。
 *
 * 真机实测:libdmhost.so 先被 ArkTS import 初始化(那时 exports 里没有
 * OH_NATIVE_XCOMPONENT_OBJ),之后 XComponent 创建时 ArkUI 复用了已初始化的
 * 模块、不再调 Init,于是 OnSurfaceCreated 永远到不了宿主,g_window 一直为空
 * (EGL/Vulkan 都拿不到窗口 → 黑屏)。
 *
 * 这里把 XComponent 的 napi 绑定单独放进一个只由 XComponent 加载的 .so:
 * 它只做回调注册,收到 surface 后调用 libdmhost 的 dmVkStart/dmVkStop。
 */
#include "napi/native_api.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <dlfcn.h>
#include <hilog/log.h>
#include <native_window/external_window.h>

typedef int (*DmVkStartFn)(OHNativeWindow *window);
typedef void (*DmVkStopFn)(void);

/* libdmhost.so 由 ArkTS import 提前加载;这里 dlsym 取它的导出,避免两个
 * napi 模块之间硬链接(也避免把 70MB 客体段重复映射)。 */
static void *DmhostHandle(void)
{
    static void *h = nullptr;
    if (h == nullptr) {
        h = dlopen("libdmhost.so", RTLD_NOW);
    }
    return h;
}

static DmVkStartFn GetVkStart(void)
{
    static DmVkStartFn fn = nullptr;
    if (fn == nullptr) {
        fn = reinterpret_cast<DmVkStartFn>(dlsym(RTLD_DEFAULT, "dmVkStart"));
        if (fn == nullptr) {
            void *h = DmhostHandle();
            if (h != nullptr) {
                fn = reinterpret_cast<DmVkStartFn>(dlsym(h, "dmVkStart"));
            }
        }
    }
    return fn;
}

static DmVkStopFn GetVkStop(void)
{
    static DmVkStopFn fn = nullptr;
    if (fn == nullptr) {
        fn = reinterpret_cast<DmVkStopFn>(dlsym(RTLD_DEFAULT, "dmVkStop"));
        if (fn == nullptr) {
            void *h = DmhostHandle();
            if (h != nullptr) {
                fn = reinterpret_cast<DmVkStopFn>(dlsym(h, "dmVkStop"));
            }
        }
    }
    return fn;
}

#define XC_TAG "D2RMobileHOS"
#define XC_LOG_I(...) OH_LOG_Print(LOG_APP, LOG_INFO, 0x0000, XC_TAG, __VA_ARGS__)
#define XC_LOG_E(...) OH_LOG_Print(LOG_APP, LOG_ERROR, 0x0000, XC_TAG, __VA_ARGS__)

static void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    XC_LOG_I("dmxe: native OnSurfaceCreated");
    if (window != nullptr) {
        DmVkStartFn start = GetVkStart();
        if (start != nullptr) {
            (void)start(reinterpret_cast<OHNativeWindow *>(window));
            XC_LOG_I("dmxe: dmVkStart requested");
        } else {
            XC_LOG_E("dmxe: dmVkStart not found");
        }
    }
}

static void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    (void)window;
}

static void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    (void)window;
    XC_LOG_I("dmxe: native OnSurfaceDestroyed");
    DmVkStopFn stop = GetVkStop();
    if (stop != nullptr) {
        stop();
    }
}

static void DispatchTouchEventCB(OH_NativeXComponent *component, void *window)
{
    (void)component;
    (void)window;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    XC_LOG_I("dmxe: init");
    napi_value obj = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &obj) != napi_ok
        || obj == nullptr) {
        XC_LOG_E("dmxe: no xcomponent object in exports");
        return exports;
    }
    OH_NativeXComponent *component = nullptr;
    if (napi_unwrap(env, obj, reinterpret_cast<void **>(&component)) != napi_ok
        || component == nullptr) {
        XC_LOG_E("dmxe: unwrap failed");
        return exports;
    }
    static OH_NativeXComponent_Callback cb;
    cb.OnSurfaceCreated = OnSurfaceCreatedCB;
    cb.OnSurfaceChanged = OnSurfaceChangedCB;
    cb.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
    cb.DispatchTouchEvent = DispatchTouchEventCB;
    OH_NativeXComponent_RegisterCallback(component, &cb);
    XC_LOG_I("dmxe: callbacks registered");
    return exports;
}
EXTERN_C_END

static napi_module dmxeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "dmxe",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterDmxeModule(void)
{
    napi_module_register(&dmxeModule);
}
