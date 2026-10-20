/*
 * dm_renderer.h — XComponent 表面上的 EGL/GLES3 渲染循环。
 */
#ifndef DM_RENDERER_H
#define DM_RENDERER_H

#include <cstdint>
#include <atomic>

#include <native_window/external_window.h>

class DmRenderer {
public:
    int Start(OHNativeWindow *window);
    void Stop();
    void SetTouchPoint(float x, float y, bool active);

private:
    void RenderLoop();
    static void ThreadTrampoline(DmRenderer *self);

    std::atomic<bool> running_{false};
    std::atomic<OHNativeWindow *> window_{nullptr};
    std::atomic<float> touchX_{0.f};
    std::atomic<float> touchY_{0.f};
    std::atomic<bool> touchActive_{false};
    void *thread_ = nullptr;
};

#endif /* DM_RENDERER_H */
