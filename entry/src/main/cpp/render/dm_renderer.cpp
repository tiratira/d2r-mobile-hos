/*
 * dm_renderer.cpp — EGL/GLES3 演示渲染循环。
 *
 * 在 XComponent 提供的 OH_NativeWindow 上建 EGL 表面,画一个
 * 暗黑风的动态渐变 + 触点高亮,每帧消费 dm_input 的触控事件。
 * 这是 NVN->Vulkan 回放器就位前的表面/线程/事件通路验证。
 */
#include "render/dm_renderer.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <hilog/log.h>

#include "input/dm_input.h"

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static const char *kVS = R"(#version 300 es
in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

static const char *kFS = R"(#version 300 es
precision mediump float;
in vec2 vUV;
out vec4 oColor;
uniform float uT;
uniform vec2 uTouch;
uniform float uTouchOn;
void main() {
    float d = length(vUV - vec2(0.5));
    vec3 c = mix(vec3(0.14, 0.045, 0.028), vec3(0.015, 0.008, 0.006), smoothstep(0.0, 0.95, d));
    c += 0.035 * vec3(1.0, 0.42, 0.13) * (0.5 + 0.5 * sin(uT * 0.8 - d * 9.0));
    if (uTouchOn > 0.5) {
        float td = length(vUV - uTouch);
        c += vec3(1.0, 0.78, 0.38) * 0.35 * smoothstep(0.055, 0.045, abs(td - 0.045));
    }
    oColor = vec4(c, 1.0);
})";

static GLuint BuildProgram()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[512];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        DMLOG_E("program link failed: %{public}s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

void DmRenderer::ThreadTrampoline(DmRenderer *self)
{
    self->RenderLoop();
}

int DmRenderer::Start(OHNativeWindow *window)
{
    if (running_.load()) {
        window_ = window; /* 表面尺寸变化时复用线程,由 SwapBuffers 自适应 */
        return 0;
    }
    window_ = window;
    running_ = true;
    pthread_t t;
    if (pthread_create(&t, nullptr, [](void *arg) -> void * {
        DmRenderer::ThreadTrampoline(static_cast<DmRenderer *>(arg));
        return nullptr;
    }, this) != 0) {
        DMLOG_E("renderer thread create failed");
        running_ = false;
        return -1;
    }
    thread_ = (void *)(uintptr_t)t;
    return 0;
}

void DmRenderer::Stop()
{
    if (!running_.exchange(false)) {
        return;
    }
    if (thread_ != nullptr) {
        pthread_join((pthread_t)(uintptr_t)thread_, nullptr);
        thread_ = nullptr;
    }
    window_ = nullptr;
}

void DmRenderer::SetTouchPoint(float x, float y, bool active)
{
    touchX_ = x;
    touchY_ = y;
    touchActive_ = active;
}

void DmRenderer::RenderLoop()
{
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
        DMLOG_E("eglInitialize failed");
        return;
    }
    const EGLint cfgAttrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint num = 0;
    if (!eglChooseConfig(display, cfgAttrs, &config, 1, &num) || num < 1) {
        DMLOG_E("eglChooseConfig failed");
        eglTerminate(display);
        return;
    }
    EGLSurface surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)window_.load(), nullptr);    const EGLint ctxAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttrs);
    if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT) {
        DMLOG_E("egl surface/context create failed");
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
        return;
    }
    eglMakeCurrent(display, surface, surface, context);
    GLuint prog = BuildProgram();
    GLuint vbo = 0, vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLfloat verts[] = {
        -1.f, -1.f, 1.f, -1.f, -1.f, 1.f,
        -1.f, 1.f,  1.f, -1.f, 1.f, 1.f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    GLint aPos = glGetAttribLocation(prog, "aPos");
    glEnableVertexAttribArray(aPos);
    glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    DMLOG_I("renderer loop started (egl-gles3)");

    auto t0 = std::chrono::steady_clock::now();
    DmTouchEvent events[16];
    int frame = 0;
    while (running_.load()) {
        /* 每帧消费输入环,取最后一个活动触点做高亮 */
        int n = dmInputPollTouch(events, 16);
        for (int i = 0; i < n; i++) {
            if (events[i].phase == DM_TOUCH_UP || events[i].phase == DM_TOUCH_CANCEL) {
                touchActive_ = false;
            } else {
                SetTouchPoint(events[i].x, events[i].y, true);
            }
        }
        EGLint w = 0, h = 0;
        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        double tsec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        glUseProgram(prog);
        glUniform1f(glGetUniformLocation(prog, "uT"), (GLfloat)tsec);
        glUniform2f(glGetUniformLocation(prog, "uTouch"), touchX_.load(), touchY_.load());
        glUniform1f(glGetUniformLocation(prog, "uTouchOn"), touchActive_.load() ? 1.f : 0.f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        eglSwapBuffers(display, surface);
        if ((++frame % 300) == 0) {
            DMLOG_I("frame %{public}d (%{public}dx%{public}d)", frame, w, h);
        }
        usleep(16000); /* ~60fps 目标;正式通路由垂直同步节流 */
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    DMLOG_I("renderer loop stopped");
}
