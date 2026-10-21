/*
 * dm_vk.c — Vulkan 后端骨架:实例 + VK_OHOS_surface 表面 + 交换链 + 呈现循环。
 *
 * 上游把 NVN 命令回放为 Metal(nvn_mtl.m);HOS 的对应物是 Vulkan:NVN 录制
 * 层(gfx/nvn/hle_nvn.c 平移)之后逐命令改写到 Vulkan。本文件先把 WSI 管线
 * 跑通 —— 真机上 dmVkStart 成功即证明 Vulkan 出图通路可用,后续回放器只换
 * 每帧的录制内容(当前为清屏色动画)。
 */
#include "gfx/dm_vk.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstring>

/* hilog 的 tag/domain 宏必须在 <hilog/log.h> 之前定义,否则 dm_vk 的日志
 * 会落到默认 domain 而被过滤(排查时看不到 "vk loop running")。 */
#define LOG_DOMAIN 0x0000
#define LOG_TAG "D2RMobileHOS"
#include <hilog/log.h>

#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_thread;
static volatile int g_running = 0;
static float g_clear[4] = {-1.0f, -1.0f, -1.0f, -1.0f};   /* <0 = 演示动画 */
static _Atomic uint64_t g_vk_frames;
static VkDevice g_upload_dev;
static VkQueue g_upload_queue;
static VkPhysicalDevice g_upload_pd;
static VkPipeline g_gpipe;
static VkPipelineLayout g_gpipe_layout;
static VkRenderPass g_gpipe_rp;
static int g_draw_test;
static VkDescriptorSet g_gpipe_set;
static VkDescriptorPool g_dummy_pool;
static VkBuffer g_dummy_buf;
static VkDeviceMemory g_dummy_buf_mem;
static VkImage g_dummy_img;
static VkDeviceMemory g_dummy_img_mem;
static VkImageView g_dummy_view;
static VkSampler g_dummy_samp;   /* 实验:M4 首绘开关(缺省关,保持已验证的清屏通路) */
extern "C" uint64_t dmVkLoadShaderModule(const char *path);
extern "C" uint64_t dmVkCreateComputePipeline(const char *path);
extern "C" uint64_t dmVkCreateGraphicsPipeline(const char *vsPath, const char *psPath);

extern "C" void dmVkSetClearColor(float r, float g, float b, float a)
{
    g_clear[0] = r;
    g_clear[1] = g;
    g_clear[2] = b;
    g_clear[3] = a;
}

static const char *vkerr(VkResult r)
{
    switch (r) {
        case VK_SUCCESS: return "ok";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "out-of-host-memory";
        case VK_ERROR_INITIALIZATION_FAILED: return "init-failed";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "ext-not-present";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "window-in-use";
        case VK_ERROR_OUT_OF_DATE_KHR: return "out-of-date";
        case VK_SUBOPTIMAL_KHR: return "suboptimal";
        default: return "other";
    }
}

const char *dmVkProbe(void)
{
    static char report[160];
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "dmhost-hos";
    app.apiVersion = VK_API_VERSION_1_1;
    const char *exts[] = {VK_KHR_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = exts;
    VkInstance inst = VK_NULL_HANDLE;
    VkResult r = vkCreateInstance(&ci, nullptr, &inst);
    if (r != VK_SUCCESS) {
        snprintf(report, sizeof(report), "vulkan: instance FAILED (%s)", vkerr(r));
        return report;
    }
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(inst, &n, nullptr);
    vkDestroyInstance(inst, nullptr);
    snprintf(report, sizeof(report), "vulkan: instance OK, %u physical device(s)", n);
    return report;
}

struct VkLoop {
    VkInstance inst;
    VkSurfaceKHR surf;
    VkDevice dev;
    VkQueue queue;
    VkSwapchainKHR swc;
    VkSemaphore semAcq, semPres;
    uint32_t qfam;
};

static void vk_teardown(struct VkLoop *v)
{
    g_upload_dev = VK_NULL_HANDLE; g_upload_queue = VK_NULL_HANDLE; g_upload_pd = VK_NULL_HANDLE;
    if (v->dev) {
        vkDeviceWaitIdle(v->dev);
    }
    if (v->semAcq) vkDestroySemaphore(v->dev, v->semAcq, nullptr);
    if (v->semPres) vkDestroySemaphore(v->dev, v->semPres, nullptr);
    if (v->swc) vkDestroySwapchainKHR(v->dev, v->swc, nullptr);
    if (v->dev) vkDestroyDevice(v->dev, nullptr);
    if (v->surf) vkDestroySurfaceKHR(v->inst, v->surf, nullptr);
    if (v->inst) vkDestroyInstance(v->inst, nullptr);
    memset(v, 0, sizeof(*v));
}

static void *vk_loop(void *arg)
{
    OHNativeWindow *window = (OHNativeWindow *)arg;
    struct VkLoop v;
    memset(&v, 0, sizeof(v));

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "dmhost-hos";
    app.apiVersion = VK_API_VERSION_1_1;
    const char *exts[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_OHOS_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = 2;
    ci.ppEnabledExtensionNames = exts;
    if (vkCreateInstance(&ci, nullptr, &v.inst) != VK_SUCCESS) {
        DMLOG_E("vk: instance failed");
        return nullptr;
    }
    VkSurfaceCreateInfoOHOS sci{};
    sci.sType = VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS;
    sci.window = window;
    if (vkCreateSurfaceOHOS(v.inst, &sci, nullptr, &v.surf) != VK_SUCCESS) {
        DMLOG_E("vk: surface failed");
        vk_teardown(&v);
        return nullptr;
    }
    uint32_t ndev = 0;
    vkEnumeratePhysicalDevices(v.inst, &ndev, nullptr);
    if (ndev == 0) {
        DMLOG_E("vk: no physical device");
        vk_teardown(&v);
        return nullptr;
    }
    VkPhysicalDevice pd;
    vkEnumeratePhysicalDevices(v.inst, &ndev, &pd);
    uint32_t nq = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, nullptr);
    VkQueueFamilyProperties qf[8];
    if (nq > 8) nq = 8;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf);
    VkBool32 present = 0;
    v.qfam = 0;
    for (uint32_t i = 0; i < nq; i++) {
        vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, v.surf, &present);
        if (present && (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            v.qfam = i;
            break;
        }
    }
    if (!present) {
        DMLOG_E("vk: no present queue");
        vk_teardown(&v);
        return nullptr;
    }
    float prio = 1.f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = v.qfam;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    const char *devexts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = devexts;
    if (vkCreateDevice(pd, &dci, nullptr, &v.dev) != VK_SUCCESS) {
        DMLOG_E("vk: device failed");
        vk_teardown(&v);
        return nullptr;
    }
    vkGetDeviceQueue(v.dev, v.qfam, 0, &v.queue);
    g_upload_dev = v.dev; g_upload_queue = v.queue; g_upload_pd = pd;
    {
        VkPhysicalDeviceProperties pr{};
        vkGetPhysicalDeviceProperties(pd, &pr);
        FILE *f = fopen("/data/storage/el2/base/haps/entry/files/vk_info.txt", "w");
        if (f != NULL) {
            fprintf(f, "device=%s api=%u.%u.%u driver=0x%08x vendor=0x%04x\n",
                    pr.deviceName, VK_VERSION_MAJOR(pr.apiVersion), VK_VERSION_MINOR(pr.apiVersion),
                    VK_VERSION_PATCH(pr.apiVersion), pr.driverVersion, pr.vendorID);
            fclose(f);
        }
    }
    DMLOG_I("M3 shader module test=%{public}llu",
            (unsigned long long)dmVkLoadShaderModule("/data/storage/el2/base/haps/entry/files/test.spv"));
    DMLOG_I("M3 compute pipeline test=%{public}llu",
            (unsigned long long)dmVkCreateComputePipeline("/data/storage/el2/base/haps/entry/files/test_cs.spv"));
    DMLOG_I("M3 graphics pipeline test=%{public}llu",
            (unsigned long long)dmVkCreateGraphicsPipeline("/data/storage/el2/base/haps/entry/files/test_vs.spv",
                                                           "/data/storage/el2/base/haps/entry/files/test_ps.spv"));

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, v.surf, &caps);
    uint32_t want = 2;
    if (caps.minImageCount > want) want = caps.minImageCount;
    if (caps.maxImageCount && caps.maxImageCount < want) want = caps.maxImageCount;

    /* 真机 swapchain 只接受它列出的 format(日志里有 "format 44 unsupported");
     * 不能硬编码 B8G8R8A8,否则 vkCreateSwapchainKHR 直接失败。 */
    VkSurfaceFormatKHR chosen{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    uint32_t nfmt = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, v.surf, &nfmt, nullptr);
    if (nfmt > 0) {
        VkSurfaceFormatKHR fmts[8];
        uint32_t n = nfmt > 8 ? 8 : nfmt;
        vkGetPhysicalDeviceSurfaceFormatsKHR(pd, v.surf, &n, fmts);
        chosen = fmts[0];
        for (uint32_t i = 0; i < n; i++) {
            if (fmts[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                fmts[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                fmts[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
                chosen = fmts[i];
                break;
            }
        }
    }
    DMLOG_I("vk: surface format=%{public}d colorspace=%{public}d count=%{public}u",
            (int)chosen.format, (int)chosen.colorSpace, nfmt);

    VkSwapchainCreateInfoKHR swci{};
    swci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swci.surface = v.surf;
    swci.minImageCount = want;
    swci.imageFormat = chosen.format;
    swci.imageColorSpace = chosen.colorSpace;
    swci.imageExtent = caps.currentExtent;
    swci.imageArrayLayers = 1;
    swci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swci.preTransform = caps.currentTransform;
    swci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swci.clipped = VK_TRUE;
    if (vkCreateSwapchainKHR(v.dev, &swci, nullptr, &v.swc) != VK_SUCCESS) {
        DMLOG_E("vk: swapchain failed");
        vk_teardown(&v);
        return nullptr;
    }
    VkSemaphoreCreateInfo semi{};
    semi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(v.dev, &semi, nullptr, &v.semAcq);
    vkCreateSemaphore(v.dev, &semi, nullptr, &v.semPres);
    DMLOG_I("vk loop running (qfam=%{public}u %{public}ux%{public}u images=%{public}u)",
            v.qfam, swci.imageExtent.width, swci.imageExtent.height, want);

    auto t0 = std::chrono::steady_clock::now();
    while (g_running) {
        uint32_t idx = 0;
        VkResult r = vkAcquireNextImageKHR(v.dev, v.swc, UINT64_MAX, v.semAcq, VK_NULL_HANDLE, &idx);
        if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR) {
            g_vk_frames++;
        }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            DMLOG_E("vk: acquire %{public}s", vkerr(r));
            break;
        }
        double t = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        float c[4] = {
            0.10f + 0.05f * (float)__builtin_sin(t * 0.9),
            0.04f,
            0.03f,
            1.0f
        };
        if (g_clear[0] >= 0.0f) {
            memcpy(c, g_clear, sizeof c);
            memset(g_clear, 0, sizeof g_clear);
            g_clear[0] = -1.0f;
        }
        VkImage img;
        uint32_t nimg = 0;
        /* 必须先给 count 指针;传 nullptr 的调用在 OHOS swapchain 实现里会
         * 直接解引用空指针(libvulkan_swapchain GetSwapchainImagesKHR+88)。 */
        vkGetSwapchainImagesKHR(v.dev, v.swc, &nimg, nullptr);
        VkImage imgs[8];
        if (nimg > 8) nimg = 8;
        if (nimg == 0) {
            DMLOG_E("vk: no swapchain images");
            break;
        }
        vkGetSwapchainImagesKHR(v.dev, v.swc, &nimg, imgs);
        img = imgs[idx % nimg];
        static VkImageView s_views[8];
        static VkFramebuffer s_fbs[8];
        if (g_gpipe_rp && s_views[idx % nimg] == VK_NULL_HANDLE) {
            VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vci.image = img; vci.viewType = VK_IMAGE_VIEW_TYPE_2D; vci.format = chosen.format;
            vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            if (vkCreateImageView(v.dev, &vci, nullptr, &s_views[idx % nimg]) == VK_SUCCESS) {
                VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
                fci.renderPass = g_gpipe_rp; fci.attachmentCount = 1; fci.pAttachments = &s_views[idx % nimg];
                fci.width = swci.imageExtent.width; fci.height = swci.imageExtent.height; fci.layers = 1;
                vkCreateFramebuffer(v.dev, &fci, nullptr, &s_fbs[idx % nimg]);
            }
        }

        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo pci{};
        pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pci.queueFamilyIndex = v.qfam;
        vkCreateCommandPool(v.dev, &pci, nullptr, &pool);
        VkCommandBuffer cmdb;
        VkCommandBufferAllocateInfo cai{};
        cai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cai.commandPool = pool;
        cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cai.commandBufferCount = 1;
        vkAllocateCommandBuffers(v.dev, &cai, &cmdb);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmdb, &bi);
        if (g_draw_test && g_gpipe != VK_NULL_HANDLE && s_fbs[idx % nimg] != VK_NULL_HANDLE) {
            VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            rbi.renderPass = g_gpipe_rp; rbi.framebuffer = s_fbs[idx % nimg];
            rbi.renderArea = {{0, 0}, {swci.imageExtent.width, swci.imageExtent.height}};
            VkClearValue cv{}; memcpy(cv.color.float32, c, sizeof c);
            rbi.clearValueCount = 1; rbi.pClearValues = &cv;
            vkCmdBeginRenderPass(cmdb, &rbi, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_gpipe);
            if (g_gpipe_set != VK_NULL_HANDLE) vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_gpipe_layout, 0, 1, &g_gpipe_set, 0, nullptr);
            VkViewport vpt{0, 0, (float)swci.imageExtent.width, (float)swci.imageExtent.height, 0, 1};
            VkRect2D sc{{0, 0}, {swci.imageExtent.width, swci.imageExtent.height}};
            vkCmdSetViewport(cmdb, 0, 1, &vpt);
            vkCmdSetScissor(cmdb, 0, 1, &sc);
            vkCmdDraw(cmdb, 3, 1, 0, 0);
            vkCmdEndRenderPass(cmdb);
            vkEndCommandBuffer(cmdb);
            VkSubmitInfo si2{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            si2.waitSemaphoreCount = 1; si2.pWaitSemaphores = &v.semAcq;
            VkPipelineStageFlags ws = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            si2.pWaitDstStageMask = &ws;
            si2.commandBufferCount = 1; si2.pCommandBuffers = &cmdb;
            si2.signalSemaphoreCount = 1; si2.pSignalSemaphores = &v.semPres;
            vkQueueSubmit(v.queue, 1, &si2, VK_NULL_HANDLE);
            VkPresentInfoKHR pi2{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            pi2.waitSemaphoreCount = 1; pi2.pWaitSemaphores = &v.semPres;
            pi2.swapchainCount = 1; pi2.pSwapchains = &v.swc; pi2.pImageIndices = &idx;
            VkResult pr = vkQueuePresentKHR(v.queue, &pi2);
            vkQueueWaitIdle(v.queue);
            vkDestroyCommandPool(v.dev, pool, nullptr);
            if (pr != VK_SUCCESS && pr != VK_SUBOPTIMAL_KHR) { DMLOG_E("vk: present %{public}s", vkerr(pr)); break; }
            usleep(16000);
            continue;
        }
        VkImageMemoryBarrier bar{};
        bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        bar.srcAccessMask = 0;
        bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bar.image = img;
        bar.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmdb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar);
        vkCmdClearColorImage(cmdb, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             (const VkClearColorValue *)c, 1, &bar.subresourceRange);
        VkImageMemoryBarrier bar2 = bar;
        bar2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bar2.dstAccessMask = 0;
        bar2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        bar2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vkCmdPipelineBarrier(cmdb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &bar2);
        vkEndCommandBuffer(cmdb);
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &v.semAcq;
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        si.pWaitDstStageMask = &waitStage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmdb;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &v.semPres;
        vkQueueSubmit(v.queue, 1, &si, VK_NULL_HANDLE);
        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = &v.semPres;
        pi.swapchainCount = 1;
        pi.pSwapchains = &v.swc;
        pi.pImageIndices = &idx;
        r = vkQueuePresentKHR(v.queue, &pi);
        vkQueueWaitIdle(v.queue);
        vkDestroyCommandPool(v.dev, pool, nullptr);
        if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR) {
            g_vk_frames++;
        }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            DMLOG_E("vk: present %{public}s", vkerr(r));
            break;
        }
        usleep(16000);
    }
    vk_teardown(&v);
    DMLOG_I("vk loop stopped");
    return nullptr;
}

extern "C" __attribute__((visibility("default"))) int dmVkStart(OHNativeWindow *window)
{
    pthread_mutex_lock(&g_mu);
    if (g_running) {
        pthread_mutex_unlock(&g_mu);
        return 0;
    }
    g_running = 1;
    if (pthread_create(&g_thread, nullptr, vk_loop, window) != 0) {
        g_running = 0;
        pthread_mutex_unlock(&g_mu);
        return -1;
    }
    pthread_mutex_unlock(&g_mu);
    return 0;
}

extern "C" __attribute__((visibility("default"))) void dmVkStop(void)
{
    pthread_mutex_lock(&g_mu);
    if (!g_running) {
        pthread_mutex_unlock(&g_mu);
        return;
    }
    g_running = 0;
    pthread_mutex_unlock(&g_mu);
    pthread_join(g_thread, nullptr);
}

extern "C" __attribute__((visibility("default"))) int dmVkRunning(void)
{
    return g_running;
}



extern "C" __attribute__((visibility("default"))) uint64_t dmVkFrames(void)
{
    return g_vk_frames;
}


/* ================= M2-upload:Tegra 纹理 → Vulkan 图像 =================
 * 回放线程在 nvnQueueSubmitCommands 时调用:线性像素(已 decodable/deswizzle)→
 * VkImage(device-local, SAMPLED|TRANSFER_DST)。返回不透明句柄(当前为 VkImage
 * 地址),失败 0。上传走临时 command pool + staging buffer,同步等待完成。 */
extern "C" uint64_t dmVkUploadTexture(const void *pixels, uint32_t w, uint32_t h,
                                      uint32_t vkfmt, uint32_t bpp);
extern "C" uint64_t dmVkUploadCount(void);
static _Atomic uint64_t g_tex_uploads;
extern "C" uint64_t dmVkUploadCount(void) { return g_tex_uploads; }

static int vk_mem_type(uint32_t bits, VkMemoryPropertyFlags want)
{
    VkPhysicalDeviceMemoryProperties mp{};
    vkGetPhysicalDeviceMemoryProperties(g_upload_pd, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) return (int)i;
    }
    return -1;
}

static int vk_buf_create(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer *out,
                         VkDeviceMemory *mem, bool host)
{
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = size; bci.usage = usage; bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(g_upload_dev, &bci, nullptr, out) != VK_SUCCESS) return -1;
    VkMemoryRequirements mr{}; vkGetBufferMemoryRequirements(g_upload_dev, *out, &mr);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; mai.allocationSize = mr.size;
    int mi = vk_mem_type(mr.memoryTypeBits, host ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                                                : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mi < 0) return -1;
    mai.memoryTypeIndex = (uint32_t)mi;
    if (vkAllocateMemory(g_upload_dev, &mai, nullptr, mem) != VK_SUCCESS) return -1;
    vkBindBufferMemory(g_upload_dev, *out, *mem, 0);
    return 0;
}

extern "C" uint64_t dmVkUploadTexture(const void *pixels, uint32_t w, uint32_t h,
                                      uint32_t vkfmt, uint32_t bpp)
{
    if (g_upload_dev == VK_NULL_HANDLE || pixels == NULL || w == 0 || h == 0 || bpp == 0) return 0;
    uint64_t bytes = (uint64_t)w * h * bpp;
    VkImage img = VK_NULL_HANDLE; VkDeviceMemory imem = VK_NULL_HANDLE;
    VkBuffer stg = VK_NULL_HANDLE; VkDeviceMemory smem = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE; VkCommandBuffer cb = VK_NULL_HANDLE;
    uint64_t r = 0;
    auto cleanup = [&]() {
        if (cb) { vkFreeCommandBuffers(g_upload_dev, pool, 1, &cb); cb = VK_NULL_HANDLE; }
        if (pool) { vkDestroyCommandPool(g_upload_dev, pool, nullptr); pool = VK_NULL_HANDLE; }
        if (stg) { vkDestroyBuffer(g_upload_dev, stg, nullptr); stg = VK_NULL_HANDLE; }
        if (smem) { vkFreeMemory(g_upload_dev, smem, nullptr); smem = VK_NULL_HANDLE; }
        if (img) { vkDestroyImage(g_upload_dev, img, nullptr); img = VK_NULL_HANDLE; }
        if (imem) { vkFreeMemory(g_upload_dev, imem, nullptr); imem = VK_NULL_HANDLE; }
    };
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType = VK_IMAGE_TYPE_2D; ici.format = (VkFormat)vkfmt;
    ici.extent = { w, h, 1 }; ici.mipLevels = 1; ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT; ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(g_upload_dev, &ici, nullptr, &img) != VK_SUCCESS) { cleanup(); return 0; }
    VkMemoryRequirements imr{}; vkGetImageMemoryRequirements(g_upload_dev, img, &imr);
    VkMemoryAllocateInfo imai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; imai.allocationSize = imr.size;
    int imi = vk_mem_type(imr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imi < 0) { cleanup(); return 0; }
    imai.memoryTypeIndex = (uint32_t)imi;
    if (vkAllocateMemory(g_upload_dev, &imai, nullptr, &imem) != VK_SUCCESS) { cleanup(); return 0; }
    vkBindImageMemory(g_upload_dev, img, imem, 0);
    if (vk_buf_create(bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &stg, &smem, true) != 0) { cleanup(); return 0; }
    void *map = nullptr;
    if (vkMapMemory(g_upload_dev, smem, 0, bytes, 0, &map) != VK_SUCCESS) { cleanup(); return 0; }
    memcpy(map, pixels, (size_t)bytes);
    vkUnmapMemory(g_upload_dev, smem);
    {
        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pci.queueFamilyIndex = 0;
        if (vkCreateCommandPool(g_upload_dev, &pci, nullptr, &pool) != VK_SUCCESS) { cleanup(); return 0; }
        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(g_upload_dev, &cai, &cb) != VK_SUCCESS) { cleanup(); return 0; }
        VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);
        VkImageMemoryBarrier b1{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b1.image = img; b1.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        b1.srcAccessMask = 0; b1.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b1);
        VkBufferImageCopy bic{}; bic.bufferOffset = 0;
        bic.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        bic.imageExtent = { w, h, 1 };
        vkCmdCopyBufferToImage(cb, stg, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);
        VkImageMemoryBarrier b2 = b1;
        b2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &b2);
        vkEndCommandBuffer(cb);
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1; si.pCommandBuffers = &cb;
        if (vkQueueSubmit(g_upload_queue, 1, &si, VK_NULL_HANDLE) == VK_SUCCESS) {
            vkQueueWaitIdle(g_upload_queue);
            g_tex_uploads++;
            r = (uint64_t)(uintptr_t)img;   /* 句柄:回放层据此建 VkImageView */
            img = VK_NULL_HANDLE;           /* 不销毁 */
        }
    }
    return r;
}


/* ================= M3-1:SPIR-V 着色器模块加载 =================
 * 校验阶段:从文件读入离线翻译的 <key>.spv,创建 VkShaderModule 再销毁,
 * 证明「6704 份 SPIR-V 资产 → Vulkan」这条链在真机上可用。 */
extern "C" uint64_t dmVkLoadShaderModule(const char *path);
extern "C" uint64_t dmVkShaderTestCount(void);
static _Atomic uint64_t g_shader_tests;
extern "C" uint64_t dmVkShaderTestCount(void) { return g_shader_tests; }

extern "C" uint64_t dmVkLoadShaderModule(const char *path)
{
    if (g_upload_dev == VK_NULL_HANDLE || path == NULL) return 0;
    FILE *f = fopen(path, "rb");
    if (f == NULL) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 20 || n > (1 << 24)) { fclose(f); return 0; }
    void *buf = malloc((size_t)n);
    if (buf == NULL) { fclose(f); return 0; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return 0; }
    fclose(f);
    uint32_t magic = 0;
    memcpy(&magic, buf, 4);
    if (magic != 0x07230203u) { free(buf); return 0; }
    VkShaderModuleCreateInfo sci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    sci.codeSize = (size_t)n;
    sci.pCode = (const uint32_t *)buf;
    VkShaderModule mod = VK_NULL_HANDLE;
    VkResult r = vkCreateShaderModule(g_upload_dev, &sci, nullptr, &mod);
    free(buf);
    if (r != VK_SUCCESS || mod == VK_NULL_HANDLE) return 0;
    vkDestroyShaderModule(g_upload_dev, mod, nullptr);
    g_shader_tests++;
    return 1;
}


/* ================= M3-2:真实 SPIR-V → VkPipeline =================
 * 取离线翻译的 compute 着色器:VkShaderModule + 空 descriptor/pipeline layout
 * + VkComputePipeline(证明「SPIR-V 资产 → 管线」链路)。返回 1 成功。 */
extern "C" uint64_t dmVkCreateComputePipeline(const char *path);
extern "C" uint64_t dmVkCreateGraphicsPipeline(const char *vsPath, const char *psPath);
extern "C" uint64_t dmVkPipelineCount(void);
static _Atomic uint64_t g_pipelines;
static _Atomic int g_pipe_err = -123;
static _Atomic int g_dsl_bindings;
extern "C" uint64_t dmVkPipelineCount(void) { return g_pipelines; }
extern "C" int dmVkPipelineError(void) { return g_pipe_err; }
extern "C" int dmVkDslBindings(void) { return g_dsl_bindings; }

/* M3-3:从 dmsh_xlate 的 .res.json 反射里抽 binding(按段落判定类型),
 * 生成真实 VkDescriptorSetLayout。返回绑定数;json 不存在返回 0。 */
static int dsl_bindings(const char *json, VkDescriptorSetLayoutBinding *out, uint32_t maxn)
{
    const char *secs[][2] = {{"cbufs",0},{"sbufs",0},{"textures",0},{"samplers",0},{"images",0}};
    VkDescriptorType types[5] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLER,
                                 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE};
    int n = 0; int cur = -1;
    for (const char *p = json; *p; p++) {
        for (int k = 0; k < 5; k++) {
            size_t l = strlen(secs[k][0]);
            if (strncmp(p, secs[k][0], l) == 0 && p[-1] == '"') cur = k;
        }
        if (strncmp(p, "\"binding\"", 9) == 0 && cur >= 0 && n < (int)maxn) {
            const char *q = strchr(p, ':'); if (!q) break;
            int b = atoi(q + 1);
            out[n].binding = (uint32_t)b; out[n].descriptorType = types[cur];
            out[n].descriptorCount = 1; out[n].stageFlags = VK_SHADER_STAGE_ALL; n++;
        }
    }
    return n;
}

extern "C" uint64_t dmVkCreateComputePipeline(const char *path)
{
    if (g_upload_dev == VK_NULL_HANDLE || path == NULL) return 0;
    FILE *f = fopen(path, "rb");
    if (f == NULL) return 0;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 20 || n > (1 << 24)) { fclose(f); return 0; }
    void *buf = malloc((size_t)n);
    if (buf == NULL) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)n, f); fclose(f);
    if (got != (size_t)n) { free(buf); return 0; }
    VkShaderModuleCreateInfo sci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    sci.codeSize = (size_t)n; sci.pCode = (const uint32_t *)buf;
    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(g_upload_dev, &sci, nullptr, &mod) != VK_SUCCESS) { free(buf); return 0; }
    VkDescriptorSetLayoutBinding binds[64] = {};
    int nb = 0;
    {
        char jp[512]; snprintf(jp, sizeof jp, "%s.res.json", path);
        FILE *jf = fopen(jp, "rb");
        if (jf != NULL) {
            static char jb[65536];
            size_t jn = fread(jb, 1, sizeof(jb) - 1, jf); fclose(jf);
            jb[jn] = 0;
            nb = dsl_bindings(jb, binds, 64);
        }
    }
    VkDescriptorSetLayoutCreateInfo dlci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    if (nb > 0) { dlci.bindingCount = (uint32_t)nb; dlci.pBindings = binds; }
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(g_upload_dev, &dlci, nullptr, &dsl);
    g_dsl_bindings = nb;
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1; plci.pSetLayouts = &dsl;
    VkPipelineLayout pl = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(g_upload_dev, &plci, nullptr, &pl) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(g_upload_dev, dsl, nullptr);
        vkDestroyShaderModule(g_upload_dev, mod, nullptr);
        free(buf);
        return 0;
    }
    VkComputePipelineCreateInfo cpci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    cpci.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    cpci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cpci.stage.module = mod;
    cpci.stage.pName = "main";
    cpci.layout = pl;
    VkPipeline pipe = VK_NULL_HANDLE;
    VkResult r = vkCreateComputePipelines(g_upload_dev, VK_NULL_HANDLE, 1, &cpci, nullptr, &pipe);
    if (pipe) vkDestroyPipeline(g_upload_dev, pipe, nullptr);
    vkDestroyPipelineLayout(g_upload_dev, pl, nullptr);
    vkDestroyDescriptorSetLayout(g_upload_dev, dsl, nullptr);
    vkDestroyShaderModule(g_upload_dev, mod, nullptr);
    free(buf);
    if (r != VK_SUCCESS) { g_pipe_err = (int)r; return 0; }
    g_pipe_err = 0;
    g_pipelines++;
    return 1;
}


/* ================= M3-4:图形管线(真实 vs/ps SPIR-V) ================= */
extern "C" uint64_t dmVkCreateGraphicsPipeline(const char *vsPath, const char *psPath);
extern "C" uint64_t dmVkGraphicsPipeCount(void);
extern "C" int dmVkGraphicsPipeError(void);
static _Atomic uint64_t g_gpipes;
static _Atomic int g_gpipe_err = -123;
extern "C" uint64_t dmVkGraphicsPipeCount(void) { return g_gpipes; }
extern "C" int dmVkGraphicsPipeError(void) { return g_gpipe_err; }

static VkShaderModule load_module(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) return VK_NULL_HANDLE;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 20 || n > (1 << 24)) { fclose(f); return VK_NULL_HANDLE; }
    void *buf = malloc((size_t)n);
    if (buf == NULL) { fclose(f); return VK_NULL_HANDLE; }
    size_t got = fread(buf, 1, (size_t)n, f); fclose(f);
    if (got != (size_t)n) { free(buf); return VK_NULL_HANDLE; }
    VkShaderModuleCreateInfo sci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    sci.codeSize = (size_t)n; sci.pCode = (const uint32_t *)buf;
    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(g_upload_dev, &sci, nullptr, &mod) != VK_SUCCESS) mod = VK_NULL_HANDLE;
    free(buf);
    return mod;
}

extern "C" uint64_t dmVkCreateGraphicsPipeline(const char *vsPath, const char *psPath)
{
    if (g_upload_dev == VK_NULL_HANDLE || !vsPath || !psPath) return 0;
    VkShaderModule vs = load_module(vsPath), ps = load_module(psPath);
    if (!vs || !ps) { if (vs) vkDestroyShaderModule(g_upload_dev, vs, nullptr);
                      if (ps) vkDestroyShaderModule(g_upload_dev, ps, nullptr); return 0; }
    VkAttachmentDescription att{};
    att.format = VK_FORMAT_R8G8B8A8_UNORM; att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ar{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1; sub.pColorAttachments = &ar;
    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 1; rpci.pAttachments = &att;
    rpci.subpassCount = 1; rpci.pSubpasses = &sub;
    VkRenderPass rp = VK_NULL_HANDLE;
    if (vkCreateRenderPass(g_upload_dev, &rpci, nullptr, &rp) != VK_SUCCESS) {
        vkDestroyShaderModule(g_upload_dev, vs, nullptr); vkDestroyShaderModule(g_upload_dev, ps, nullptr); return 0;
    }
    VkDescriptorSetLayoutBinding binds[128] = {}; int nb = 0;
    {
        char jp[512]; static char jb[65536];
        for (int side = 0; side < 2; side++) {
            snprintf(jp, sizeof jp, "%s.res.json", side == 0 ? vsPath : psPath);
            FILE *jf = fopen(jp, "rb");
            if (!jf) continue;
            size_t jn = fread(jb, 1, sizeof(jb) - 1, jf); fclose(jf); jb[jn] = 0;
            VkDescriptorSetLayoutBinding tmp[64] = {};
            int tn = dsl_bindings(jb, tmp, 64);
            for (int i = 0; i < tn && nb < 128; i++) {
                int dup = 0;
                for (int k = 0; k < nb; k++) if (binds[k].binding == tmp[i].binding) { dup = 1; break; }
                if (!dup) binds[nb++] = tmp[i];
            }
        }
    }
    VkDescriptorSetLayoutCreateInfo dlci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    if (nb > 0) { dlci.bindingCount = (uint32_t)nb; dlci.pBindings = binds; }
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    vkCreateDescriptorSetLayout(g_upload_dev, &dlci, nullptr, &dsl);
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1; plci.pSetLayouts = &dsl;
    VkPipelineLayout pl = VK_NULL_HANDLE;
    vkCreatePipelineLayout(g_upload_dev, &plci, nullptr, &pl);
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vs; st[0].pName = "main";
    st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = ps; st[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1; vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = 0xf;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1; cb.pAttachments = &cba;
    VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    ds.dynamicStateCount = 2; ds.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gpci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gpci.stageCount = 2; gpci.pStages = st;
    gpci.pVertexInputState = &vi; gpci.pInputAssemblyState = &ia;
    gpci.pViewportState = &vp; gpci.pRasterizationState = &rs;
    gpci.pMultisampleState = &ms; gpci.pColorBlendState = &cb;
    gpci.pDynamicState = &ds; gpci.layout = pl; gpci.renderPass = rp; gpci.subpass = 0;
    VkPipeline pipe = VK_NULL_HANDLE;
    VkResult r = vkCreateGraphicsPipelines(g_upload_dev, VK_NULL_HANDLE, 1, &gpci, nullptr, &pipe);
    if (r == VK_SUCCESS && pipe != VK_NULL_HANDLE) {
        g_gpipe = pipe; g_gpipe_layout = pl; g_gpipe_rp = rp;
        /* M4:为每个 binding 写占位资源(统一缓冲=4KB buffer,纹理=1x1 白图+采样器) */
        {
            VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bci.size = 4096; bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            if (vkCreateBuffer(g_upload_dev, &bci, nullptr, &g_dummy_buf) == VK_SUCCESS) {
                VkMemoryRequirements mr{}; vkGetBufferMemoryRequirements(g_upload_dev, g_dummy_buf, &mr);
                VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; mai.allocationSize = mr.size;
                int mi = vk_mem_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (mi >= 0) { mai.memoryTypeIndex = (uint32_t)mi;
                    if (vkAllocateMemory(g_upload_dev, &mai, nullptr, &g_dummy_buf_mem) == VK_SUCCESS) {
                        vkBindBufferMemory(g_upload_dev, g_dummy_buf, g_dummy_buf_mem, 0);
                        void *m = nullptr; if (vkMapMemory(g_upload_dev, g_dummy_buf_mem, 0, 4096, 0, &m) == VK_SUCCESS) { memset(m, 0, 4096); vkUnmapMemory(g_upload_dev, g_dummy_buf_mem); }
                    } }
            }
            static const uint8_t white[4] = {255,255,255,255};
            VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            ici.imageType = VK_IMAGE_TYPE_2D; ici.format = VK_FORMAT_R8G8B8A8_UNORM;
            ici.extent = {1,1,1}; ici.mipLevels = 1; ici.arrayLayers = 1; ici.samples = VK_SAMPLE_COUNT_1_BIT;
            ici.tiling = VK_IMAGE_TILING_OPTIMAL; ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (vkCreateImage(g_upload_dev, &ici, nullptr, &g_dummy_img) == VK_SUCCESS) {
                VkMemoryRequirements mr{}; vkGetImageMemoryRequirements(g_upload_dev, g_dummy_img, &mr);
                VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; mai.allocationSize = mr.size;
                int mi = vk_mem_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (mi >= 0) { mai.memoryTypeIndex = (uint32_t)mi;
                    if (vkAllocateMemory(g_upload_dev, &mai, nullptr, &g_dummy_img_mem) == VK_SUCCESS)
                        vkBindImageMemory(g_upload_dev, g_dummy_img, g_dummy_img_mem, 0); }
                VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                vci.image = g_dummy_img; vci.viewType = VK_IMAGE_VIEW_TYPE_2D; vci.format = VK_FORMAT_R8G8B8A8_UNORM;
                vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
                vkCreateImageView(g_upload_dev, &vci, nullptr, &g_dummy_view);
            }
            VkSamplerCreateInfo sci2{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            sci2.magFilter = VK_FILTER_NEAREST; sci2.minFilter = VK_FILTER_NEAREST;
            vkCreateSampler(g_upload_dev, &sci2, nullptr, &g_dummy_samp);
            VkDescriptorPoolSize ps[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32}};
            VkDescriptorPoolCreateInfo dpci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            dpci.maxSets = 2; dpci.poolSizeCount = 3; dpci.pPoolSizes = ps;
            if (vkCreateDescriptorPool(g_upload_dev, &dpci, nullptr, &g_dummy_pool) == VK_SUCCESS) {
                VkDescriptorSetAllocateInfo dsai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
                dsai.descriptorPool = g_dummy_pool; dsai.descriptorSetCount = 1; dsai.pSetLayouts = &dsl;
                if (vkAllocateDescriptorSets(g_upload_dev, &dsai, &g_gpipe_set) == VK_SUCCESS) {
                    VkDescriptorBufferInfo bi{g_dummy_buf, 0, 4096};
                    VkDescriptorImageInfo ii{g_dummy_samp, g_dummy_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                    VkWriteDescriptorSet wr[32]{}; uint32_t nw = 0;
                    for (int k = 0; k < nb && nw < 32; k++) {
                        VkWriteDescriptorSet *w = &wr[nw];
                        w->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w->dstSet = g_gpipe_set;
                        w->dstBinding = binds[k].binding; w->descriptorCount = 1;
                        if (binds[k].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                            binds[k].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                            binds[k].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                            w->descriptorType = binds[k].descriptorType; w->pImageInfo = &ii;
                        } else {
                            w->descriptorType = binds[k].descriptorType; w->pBufferInfo = &bi;
                        }
                        nw++;
                    }
                    if (nw) vkUpdateDescriptorSets(g_upload_dev, nw, wr, 0, nullptr);
                }
            }
        }
        vkDestroyDescriptorSetLayout(g_upload_dev, dsl, nullptr);
        vkDestroyShaderModule(g_upload_dev, vs, nullptr);
        vkDestroyShaderModule(g_upload_dev, ps, nullptr);
        g_gpipe_err = 0; g_gpipes++;
        return 1;
    }
    if (pipe) vkDestroyPipeline(g_upload_dev, pipe, nullptr);
    vkDestroyPipelineLayout(g_upload_dev, pl, nullptr);
    vkDestroyDescriptorSetLayout(g_upload_dev, dsl, nullptr);
    vkDestroyRenderPass(g_upload_dev, rp, nullptr);
    vkDestroyShaderModule(g_upload_dev, vs, nullptr);
    vkDestroyShaderModule(g_upload_dev, ps, nullptr);
    if (r != VK_SUCCESS) { g_gpipe_err = (int)r; return 0; }
    g_gpipe_err = 0; g_gpipes++;
    return 1;
}


/* ================= M4-3b:按 <key> 从设备目录加载真实着色器 ================= */
extern "C" int dmVkShaderStageOfFile(const char *path);
extern "C" int dmVkShaderStageOfFile(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) return -1;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 20 || n > (1 << 24)) { fclose(f); return -1; }
    uint32_t *w = (uint32_t *)malloc((size_t)n);
    if (w == NULL) { fclose(f); return -1; }
    size_t got = fread(w, 1, (size_t)n, f); fclose(f);
    if (got != (size_t)n || w[0] != 0x07230203u) { free(w); return -1; }
    int stage = -1;
    size_t wc_total = (size_t)got / 4, i = 5;
    while (i < wc_total) {
        uint32_t wc = w[i] >> 16, op = w[i] & 0xffff;
        if (op == 15 && wc >= 2) { stage = (int)w[i + 1]; break; }  /* OpEntryPoint: ExecutionModel */
        if (wc == 0) break;
        i += wc;
    }
    free(w);
    return stage;   /* 0=Vertex, 4=Fragment, 5=GLCompute */
}


/* ================= M4-4:pen(程序键对) → 管线缓存 ================= */
extern "C" uint64_t dmVkPipelineForKeys(const char *k0, const char *k1);
extern "C" int dmVkPipelineCacheCount(void);
typedef struct { char k0[17], k1[17]; VkPipeline pipe; VkPipelineLayout layout; VkRenderPass rp; } keypipe_t;
static keypipe_t g_kp[64];
static int g_kp_n;
extern "C" int dmVkPipelineCacheCount(void) { return g_kp_n; }
extern "C" uint64_t dmVkCachedPipeline(const char *k0, const char *k1);
extern "C" uint64_t dmVkCachedPipeline(const char *k0, const char *k1)
{
    if (k0 == NULL || k1 == NULL) return 0;
    for (int i = 0; i < g_kp_n; i++)
        if (strcmp(g_kp[i].k0, k0) == 0 && strcmp(g_kp[i].k1, k1) == 0)
            return (uint64_t)(uintptr_t)g_kp[i].pipe;
    return 0;
}
extern "C" int dmVkPipelineObjCount(void);
extern "C" int dmVkPipelineObjCount(void)
{
    int n = 0;
    for (int i = 0; i < g_kp_n; i++) if (g_kp[i].pipe != VK_NULL_HANDLE) n++;
    return n;
}

extern "C" uint64_t dmVkPipelineForKeys(const char *k0, const char *k1)
{
    if (k0 == NULL || k1 == NULL) return 0;
    for (int i = 0; i < g_kp_n; i++)
        if (strcmp(g_kp[i].k0, k0) == 0 && strcmp(g_kp[i].k1, k1) == 0) return (uint64_t)(g_kp[i].pipe != VK_NULL_HANDLE);
    if (g_kp_n >= 64) return 0;
    char p0[256], p1[256];
    snprintf(p0, sizeof p0, "/data/storage/el2/base/haps/entry/files/shaders/%s.spv", k0);
    snprintf(p1, sizeof p1, "/data/storage/el2/base/haps/entry/files/shaders/%s.spv", k1);
    /* 管线对象保存在 g_gpipe* 里(单探针),缓存表先记录“这些键能建出管线” */
    VkPipeline keep_pipe = g_gpipe;
    VkRenderPass keep_rp = g_gpipe_rp;
    VkPipelineLayout keep_pl = g_gpipe_layout;
    int ok = (int)dmVkCreateGraphicsPipeline(p0, p1);
    if (!ok) ok = (int)dmVkCreateGraphicsPipeline(p1, p0);
    VkPipeline made = g_gpipe; VkRenderPass made_rp = g_gpipe_rp; VkPipelineLayout made_pl = g_gpipe_layout;
    /* 探针管线保持不变 */
    g_gpipe = keep_pipe; g_gpipe_rp = keep_rp; g_gpipe_layout = keep_pl;
    if (!ok || made == VK_NULL_HANDLE) return 0;
    snprintf(g_kp[g_kp_n].k0, sizeof g_kp[0].k0, "%s", k0);
    snprintf(g_kp[g_kp_n].k1, sizeof g_kp[0].k1, "%s", k1);
    g_kp[g_kp_n].pipe = made;
    g_kp[g_kp_n].layout = made_pl;
    g_kp[g_kp_n].rp = made_rp;
    g_kp_n++;
    return 1;
}
