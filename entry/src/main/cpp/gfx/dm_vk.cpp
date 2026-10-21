/* XComponent Vulkan lifecycle. NVN rendering lives in dm_vk_replay.cpp. */
#include "gfx/dm_vk.h"
#include <cstdio>
#include <pthread.h>
#include <vulkan/vulkan.h>
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t thread;
static volatile int running;
extern "C" void dmReplayLoop(OHNativeWindow *, volatile int *);
extern "C" uint64_t dmReplayFrames(void);
static void *loop(void *window) {
    dmReplayLoop((OHNativeWindow *)window, &running);
    return nullptr;
}
extern "C" int dmVkStart(OHNativeWindow *window) {
    pthread_mutex_lock(&mutex);
    if (running) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    running = 1;
    int r = pthread_create(&thread, nullptr, loop, window);
    if (r)
        running = 0;
    pthread_mutex_unlock(&mutex);
    return r ? -1 : 0;
}
extern "C" void dmVkStop() {
    pthread_mutex_lock(&mutex);
    int join = running;
    running = 0;
    pthread_mutex_unlock(&mutex);
    if (join)
        pthread_join(thread, nullptr);
}
extern "C" int dmVkRunning() { return running; }
extern "C" uint64_t dmVkFrames() { return dmReplayFrames(); }
extern "C" void dmVkSetClearColor(float, float, float, float) {}
const char *dmVkProbe() {
    static char report[160];
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "dmhost-hos";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    VkInstance instance{};
    VkResult r = vkCreateInstance(&ci, nullptr, &instance);
    uint32_t n = 0;
    if (!r) {
        vkEnumeratePhysicalDevices(instance, &n, nullptr);
        vkDestroyInstance(instance, nullptr);
    }
    snprintf(report, sizeof report, "vulkan: result=%d, %u physical device(s), NVN replay frames=%llu", r, n,
             (unsigned long long)dmReplayFrames());
    return report;
}
