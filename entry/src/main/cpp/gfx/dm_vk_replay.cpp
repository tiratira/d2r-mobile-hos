#include "gfx/nvn/nvn_dirty.h"
// Vulkan replay of the upstream NVN record ABI. All submissions and presentation
// share one mutex; a submit returns only after GPU completion, preserving guest
// command-memory, uniform-update, event and fence lifetimes.
#include "gfx/nvn/dm_replay.h"
#include "gfx/nvn/nvn.h"
#include "gfx/nvn/nvn_obj.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <native_window/external_window.h>
#include <string>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
extern "C" {
#include "gfx/tex/nvn_bcdec.h"
#include "gfx/tex/nvn_deswizzle.h"
void dmNvnShaderKey(const void *, char out[17]);
}
#define LOG_DOMAIN 0
#define LOG_TAG "D2RMobileHOS"
#include <hilog/log.h>
#define RLOG(...) OH_LOG_Print(LOG_APP, LOG_INFO, 0, LOG_TAG, __VA_ARGS__)
namespace {
const char *base = "/data/storage/el2/base/haps/entry/files/";
std::recursive_mutex mu;
std::atomic<uint64_t> frames{0}, serial{0};
std::atomic<bool> ready{false};
VkInstance instance{};
VkPhysicalDevice physical{};
VkDevice dev{};
VkQueue queue{};
VkSurfaceKHR surface{};
VkSwapchainKHR swapchain{};
VkExtent2D extent{};
uint32_t qfamily{};
VkSemaphore acquired{}, rendered{};
VkCommandPool cmdpool{};
VkCommandBuffer cmd{};
VkDescriptorPool descpool{};
VkPhysicalDeviceProperties properties{};
struct Buffer {
    VkBuffer b{};
    VkDeviceMemory mem{};
    uint8_t *map{};
    size_t size{};
};
Buffer arena;
std::vector<Buffer> arenaPages;
size_t arenaPage{};
size_t cursor{};
struct Slice {
    VkBuffer b{};
    size_t off{}, size{};
    uint8_t *mapped{};
};
std::map<std::pair<uint64_t, uint64_t>, Slice> copies;
Slice zeroSlice{}, supportSlice{};
std::map<std::string, Slice> constantSlices;
struct Writeback {
    uint64_t addr;
    Slice slice;
};
std::vector<Writeback> writebacks;
std::vector<VkFramebuffer> framebuffers;
std::vector<VkBufferView> bufferViews;
uint64_t draws{}, dispatches{}, uploads{}, missing{}, pipelineErrors{}, submits{};
bool deviceLost{};
unsigned debugFlags{};
std::string lastGraphics, lastCompute;
FILE *diagnostic{};
void note(const char *fmt, ...) {
    if (!diagnostic)
        diagnostic = fopen((std::string(base) + "dm_replay.log").c_str(), "w");
    if (!diagnostic)
        return;
    va_list a;
    va_start(a, fmt);
    vfprintf(diagnostic, fmt, a);
    va_end(a);
    fputc('\n', diagnostic);
    fflush(diagnostic);
}
void check(VkResult r, const char *where) {
    if (r != VK_SUCCESS) {
        note("%s: VkResult=%d submit=%llu draws=%llu dispatch=%llu", where, r, (unsigned long long)submits,
             (unsigned long long)draws, (unsigned long long)dispatches);
        if (r == VK_ERROR_DEVICE_LOST) {
            deviceLost = true;
            note("last graphics=%s compute=%s", lastGraphics.c_str(), lastCompute.c_str());
        }
        RLOG("replay %{public}s: %{public}d", where, r);
    }
}
int memtype(uint32_t mask, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p;
    vkGetPhysicalDeviceMemoryProperties(physical, &p);
    for (uint32_t i = 0; i < p.memoryTypeCount; i++)
        if ((mask & (1u << i)) && (p.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return -1;
}
Buffer buffer(size_t n, VkBufferUsageFlags usage) {
    Buffer b;
    b.size = n;
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size = n;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(dev, &ci, nullptr, &b.b), "buffer");
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev, b.b, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    int m = memtype(req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (m < 0)
        abort();
    ai.memoryTypeIndex = m;
    check(vkAllocateMemory(dev, &ai, nullptr, &b.mem), "buffer memory");
    check(vkBindBufferMemory(dev, b.b, b.mem, 0), "bind memory");
    void *p{};
    check(vkMapMemory(dev, b.mem, 0, n, 0, &p), "map");
    b.map = (uint8_t *)p;
    return b;
}
Slice bytes(const void *p, size_t n) {
    n = std::max(n, size_t(16));
    size_t off = (cursor + 255) & ~size_t(255);
    if (off + n > arena.size) {
        arenaPage++;
        if (arenaPage >= arenaPages.size())
            arenaPages.push_back(buffer(
                std::max(size_t(128 << 20), n + 256),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT));
        arena = arenaPages[arenaPage];
        cursor = off = 0;
    }
    cursor = off + n;
    if (p)
        memcpy(arena.map + off, p, n);
    else
        memset(arena.map + off, 0, n);
    return {arena.b, off, n, arena.map + off};
}
Slice guest(uint64_t addr, size_t n) {
    if (!addr || !n)
        return zeroSlice.b ? zeroSlice : (zeroSlice = bytes(nullptr, 65536));
    n = std::min(n, size_t(64 << 20));
    auto key = std::make_pair(addr, (uint64_t)n);
    auto it = copies.find(key);
    if (it != copies.end())
        return it->second;
    Slice s = bytes((const void *)(uintptr_t)addr, n);
    copies.emplace(key, s);
    return s;
}
std::vector<uint8_t> read(const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return {};
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    std::vector<uint8_t> b(n > 0 ? n : 0);
    if (n > 0)
        fread(b.data(), 1, n, f);
    fclose(f);
    return b;
}
// Restricted JSON reflection reader: the translator writes arrays of flat objects.
int field(const std::string &s, const char *k, int fallback = 0) {
    auto p = s.find(std::string("\"") + k + "\"");
    if (p == s.npos)
        return fallback;
    p = s.find(':', p);
    if (p == s.npos)
        return fallback;
    const char *b = s.c_str() + p + 1;
    while (*b == ' ')
        b++;
    if (!strncmp(b, "true", 4))
        return 1;
    return strtol(b, nullptr, 10);
}
std::vector<std::string> objects(const std::string &s, const char *section) {
    std::vector<std::string> out;
    auto p = s.find(std::string("\"") + section + "\"");
    if (p == s.npos)
        return out;
    p = s.find('[', p);
    auto e = s.find(']', p);
    while ((p = s.find('{', p)) != s.npos && p < e) {
        auto q = s.find('}', p);
        if (q == s.npos)
            break;
        out.push_back(s.substr(p, q - p + 1));
        p = q + 1;
    }
    return out;
}
struct Binding {
    uint32_t binding{};
    VkDescriptorType type{};
    int kind{}, slot{-1}, cb{}, offset{}, handle{}, cbuf{-1}, count{1}, dim{1};
    bool separate{};
};
struct Shader {
    VkShaderModule module{};
    VkDescriptorSetLayout layout{};
    int stage{};
    std::string key;
    std::vector<Binding> bindings;
    std::vector<uint32_t> inputs;
    bool logged{};
    bool drawLogged{};
};
std::map<std::string, Shader> shaders;
std::map<std::string, std::vector<uint8_t>> shaderConstants;
struct Prog {
    Shader *sh[6]{};
};
std::map<uint64_t, Prog> programs;
struct Texture {
    VkImage image{};
    VkDeviceMemory memory{};
    VkImageView view{};
    VkFormat format{};
    uint32_t w{}, h{}, depth{}, layers{}, levels{1};
    int target{};
    uint64_t stamp{}, seen{};
    std::map<std::string, VkImageView> views;
    VkImageAspectFlags aspect{VK_IMAGE_ASPECT_COLOR_BIT};
};
std::map<uint64_t, Texture> textures;
std::multimap<uint64_t, uint64_t> retiredTextures;
std::vector<Texture> replacedTextures;
struct Registration {
    tex_t tex{};
    view_t view{};
    bool hasView{};
};
std::map<int, Registration> registrations;
std::map<int, samp_par> samplerParams;
std::map<int, VkSampler> samplers;
struct State {
    const prog_t *prog{};
    p_set_rt rt{};
    va_st va[16]{};
    vs_st streams[16]{};
    NVNbufferRange vb[16]{};
    p_bind_buf ub[6][16]{}, ssbo[6][16]{};
    uint64_t tex[6][128]{}, samp[6][128]{}, img[6][32]{};
    blend_st blend[8]{};
    color_st color{};
    chmask_st mask{0xffffffffu};
    ds_st ds{};
    poly_st poly{};
    p_scissor sc{};
    float viewport[4]{}, nearZ{}, farZ{1};
    int stencilRef{}, stencilMask{255}, stencilValueMask{255};
    float blendColor[4]{};
    size_t rowStride{}, imageStride{};
} S;
bool inPass{};
VkRenderPass currentPass{};
uint32_t passW{}, passH{};
void endpass() {
    if (inPass) {
        vkCmdEndRenderPass(cmd);
        inPass = false;
        VkMemoryBarrier b{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                             1, &b, 0, nullptr, 0, nullptr);
    }
}
VkFormat format(int f) {
    static const VkFormat primary[] = {VK_FORMAT_UNDEFINED,
                                       VK_FORMAT_R8_UNORM,
                                       VK_FORMAT_R8_SNORM,
                                       VK_FORMAT_R8_UINT,
                                       VK_FORMAT_R8_SINT,
                                       VK_FORMAT_R16_SFLOAT,
                                       VK_FORMAT_R16_UNORM,
                                       VK_FORMAT_R16_SNORM,
                                       VK_FORMAT_R16_UINT,
                                       VK_FORMAT_R16_SINT,
                                       VK_FORMAT_R32_SFLOAT,
                                       VK_FORMAT_R32_UINT,
                                       VK_FORMAT_R32_SINT,
                                       VK_FORMAT_R8G8_UNORM,
                                       VK_FORMAT_R8G8_SNORM,
                                       VK_FORMAT_R8G8_UINT,
                                       VK_FORMAT_R8G8_SINT,
                                       VK_FORMAT_R16G16_SFLOAT,
                                       VK_FORMAT_R16G16_UNORM,
                                       VK_FORMAT_R16G16_SNORM,
                                       VK_FORMAT_R16G16_UINT,
                                       VK_FORMAT_R16G16_SINT,
                                       VK_FORMAT_R32G32_SFLOAT,
                                       VK_FORMAT_R32G32_UINT,
                                       VK_FORMAT_R32G32_SINT,
                                       VK_FORMAT_R8G8B8_UNORM,
                                       VK_FORMAT_R8G8B8_SNORM,
                                       VK_FORMAT_R8G8B8_UINT,
                                       VK_FORMAT_R8G8B8_SINT,
                                       VK_FORMAT_R16G16B16_SFLOAT,
                                       VK_FORMAT_R16G16B16_UNORM,
                                       VK_FORMAT_R16G16B16_SNORM,
                                       VK_FORMAT_R16G16B16_UINT,
                                       VK_FORMAT_R16G16B16_SINT,
                                       VK_FORMAT_R32G32B32_SFLOAT,
                                       VK_FORMAT_R32G32B32_UINT,
                                       VK_FORMAT_R32G32B32_SINT,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_FORMAT_R8G8B8A8_SNORM,
                                       VK_FORMAT_R8G8B8A8_UINT,
                                       VK_FORMAT_R8G8B8A8_SINT,
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       VK_FORMAT_R16G16B16A16_UNORM,
                                       VK_FORMAT_R16G16B16A16_SNORM,
                                       VK_FORMAT_R16G16B16A16_UINT,
                                       VK_FORMAT_R16G16B16A16_SINT,
                                       VK_FORMAT_R32G32B32A32_SFLOAT,
                                       VK_FORMAT_R32G32B32A32_UINT,
                                       VK_FORMAT_R32G32B32A32_SINT};
    if (f > 0 && f < int(sizeof primary / sizeof *primary))
        return primary[f];
    switch (f) {
    case 0x31:
        return VK_FORMAT_S8_UINT;
    case 0x32:
        return VK_FORMAT_D16_UNORM;
    case 0x33:
    case 0x34:
        return VK_FORMAT_D32_SFLOAT;
    case 0x35:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case 0x36:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case 0x37:
    case 0x38:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case 0x3d:
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case 0x3e:
        return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case 0x3f:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case 0x40:
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
    case 0x51:
        return VK_FORMAT_R8_USCALED;
    case 0x52:
        return VK_FORMAT_R8_SSCALED;
    case 0x53:
        return VK_FORMAT_R16_USCALED;
    case 0x54:
        return VK_FORMAT_R16_SSCALED;
    case 0x55:
        return VK_FORMAT_R32_UINT;
    case 0x56:
        return VK_FORMAT_R32_SINT;
    case 0x57:
        return VK_FORMAT_R8G8_USCALED;
    case 0x58:
        return VK_FORMAT_R8G8_SSCALED;
    case 0x59:
        return VK_FORMAT_R16G16_USCALED;
    case 0x5a:
        return VK_FORMAT_R16G16_SSCALED;
    case 0x5d:
        return VK_FORMAT_R8G8B8_USCALED;
    case 0x5e:
        return VK_FORMAT_R8G8B8_SSCALED;
    case 0x5f:
        return VK_FORMAT_R16G16B16_USCALED;
    case 0x60:
        return VK_FORMAT_R16G16B16_SSCALED;
    case 0x63:
        return VK_FORMAT_R8G8B8A8_USCALED;
    case 0x64:
        return VK_FORMAT_R8G8B8A8_SSCALED;
    case 0x65:
        return VK_FORMAT_R16G16B16A16_USCALED;
    case 0x66:
        return VK_FORMAT_R16G16B16A16_SSCALED;
    default:
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
}
void imageBarrier(Texture &t, VkImageLayout old, VkImageLayout next) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = old;
    b.newLayout = next;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = t.image;
    b.subresourceRange = {t.aspect, 0, t.levels, 0, t.layers};
    b.srcAccessMask = old == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &b);
}
Texture makeTexture(const tex_t &t) {
    if (submits < 2)
        note("texture ser=%llu fmt=%d %ux%u target=%d mem=%p", (unsigned long long)t.serial, t.d.format,
             t.d.width, t.d.height, t.d.target, t.mem);
    Texture out;
    out.target = t.d.target;
    out.w = std::max(1u, t.d.width);
    out.h = std::max(1u, t.d.height);
    out.depth = t.d.target == 2 ? std::max(1u, t.d.depth) : 1;
    out.layers = (t.d.target == 4 || t.d.target == 3 || t.d.target == 9) ? std::max(1u, t.d.depth)
                 : t.d.target == 8                                       ? 6
                                                                         : 1;
    out.levels = std::max(1u, std::min(t.d.levels, 1u + (unsigned)std::log2(std::max({out.w, out.h, out.depth}))));
    int bw = 1, bh = 1, bpb = nvn_fmt_bpb(t.d.format, &bw, &bh);
    bool bc = t.d.format >= 0x41 && t.d.format <= 0x50;
    out.format = bc ? VK_FORMAT_R8G8B8A8_UNORM : format(t.d.format);
    if (bc && t.d.format == 0x49)
        out.format = VK_FORMAT_R8_UNORM;
    if (bc && t.d.format == 0x4a)
        out.format = VK_FORMAT_R8_SNORM;
    if (bc && t.d.format == 0x4b)
        out.format = VK_FORMAT_R8G8_UNORM;
    if (bc && t.d.format == 0x4c)
        out.format = VK_FORMAT_R8G8_SNORM;
    if (bc && t.d.format >= 0x4f)
        out.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    if (bc && (t.d.format == 0x45 || t.d.format == 0x46 || t.d.format == 0x47 || t.d.format == 0x48 ||
               t.d.format == 0x4e))
        out.format = VK_FORMAT_R8G8B8A8_SRGB;
    if (t.d.format >= 0x31 && t.d.format <= 0x36)
        out.aspect = t.d.format == 0x31
                         ? VK_IMAGE_ASPECT_STENCIL_BIT
                         : VK_IMAGE_ASPECT_DEPTH_BIT | (t.d.format >= 0x35 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = t.d.target == 2 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    ci.format = out.format;
    ci.extent = {out.w, out.h, out.depth};
    ci.mipLevels = out.levels;
    ci.arrayLayers = out.layers;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(physical, out.format, &fp);
    if (out.aspect != VK_IMAGE_ASPECT_COLOR_BIT)
        ci.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else {
        if (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
            ci.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
            ci.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (t.d.target == 8 || t.d.target == 9)
        ci.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    VkResult r = vkCreateImage(dev, &ci, nullptr, &out.image);
    if (r) {
        note("image fmt=%d %ux%u target=%d error=%d", t.d.format, out.w, out.h, t.d.target, r);
        return {};
    }
    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(dev, out.image, &mr);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = mr.size;
    int mt = memtype(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mt < 0)
        return {};
    ai.memoryTypeIndex = mt;
    if (vkAllocateMemory(dev, &ai, nullptr, &out.memory))
        return {};
    vkBindImageMemory(dev, out.image, out.memory, 0);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = out.image;
    vi.viewType = t.d.target == 2   ? VK_IMAGE_VIEW_TYPE_3D
                  : t.d.target == 8 ? VK_IMAGE_VIEW_TYPE_CUBE
                  : t.d.target == 9 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
                  : out.layers > 1  ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                    : VK_IMAGE_VIEW_TYPE_2D;
    vi.format = out.format;
    vi.subresourceRange = {out.aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : out.aspect, 0,
                           out.levels, 0, out.layers};
    check(vkCreateImageView(dev, &vi, nullptr, &out.view), "image view");
    imageBarrier(out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    // Packaged images carry Tegra block-linear bytes. Render targets start clear.
    if (t.mem && t.has_layout && out.aspect == VK_IMAGE_ASPECT_COLOR_BIT && bpb > 0) {
        for (unsigned level = 0; level < out.levels; level++) {
            unsigned width = std::max(1u, out.w >> level), height = std::max(1u, out.h >> level);
            unsigned depth = std::max(1u, out.depth >> level);
            size_t pitch = ((width + bw - 1) / bw) * bpb, rows = (height + bh - 1) / bh;
            for (unsigned layer = 0; layer < out.layers; layer++) {
                std::vector<uint8_t> linear(pitch * rows * depth);
                if (nvn_deswizzle(&t.d, level, layer, t.mem, linear.data(), pitch)) continue;
                Slice src;
                if (bc) {
                    uint32_t pf; int unpacked;
                    nvn_bc_unpacked(t.d.format, &pf, &unpacked);
                    std::vector<uint8_t> rgba(width * height * depth * unpacked);
                    for (unsigned z = 0; z < depth; z++)
                        nvn_bc_decode(t.d.format, linear.data() + z * pitch * rows, pitch,
                                      width, height, rgba.data() + z * width * height * unpacked, width * unpacked);
                    src = bytes(rgba.data(), rgba.size());
                } else src = bytes(linear.data(), linear.size());
                VkBufferImageCopy copy{};
                copy.bufferOffset = src.off;
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1};
                copy.imageExtent = {width, height, depth};
                vkCmdCopyBufferToImage(cmd, src.b, out.image, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
                uploads++;
            }
        }
    } else if (out.aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
        VkClearColorValue c{};
        VkImageSubresourceRange range{out.aspect, 0, out.levels, 0, out.layers};
        vkCmdClearColorImage(cmd, out.image, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &range);
    } else {
        VkClearDepthStencilValue c{1, 0};
        VkImageSubresourceRange range{out.aspect, 0, out.levels, 0, out.layers};
        vkCmdClearDepthStencilImage(cmd, out.image, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &range);
    }
    imageBarrier(out, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    return out;
}
void destroyTexture(Texture &t) {
    for (auto &v : t.views) vkDestroyImageView(dev, v.second, nullptr);
    vkDestroyImageView(dev, t.view, nullptr);
    vkDestroyImage(dev, t.image, nullptr);
    vkFreeMemory(dev, t.memory, nullptr);
}
Texture *texture(const tex_t *t) {
    if (!t || !t->serial) return nullptr;
    auto it = textures.find(t->serial);
    bool packaged = t->mem && t->has_layout;
    if (it != textures.end()) {
        uint64_t now = nvn_dirty_now();
        if (!packaged || it->second.seen == now) return &it->second;
        it->second.seen = now;
        if (!nvn_dirty_since(t->mem,t->L.size,it->second.stamp)) return &it->second;
    }
    endpass();
    uint64_t stamp = nvn_dirty_now();
    if (packaged) nvn_dirty_protect(t->mem,t->L.size);
    Texture value = makeTexture(*t);
    value.stamp = stamp; value.seen = stamp;
    if (!value.image) return nullptr;
    if (it != textures.end()) {
        replacedTextures.push_back(std::move(it->second));
        it->second = std::move(value);
        return &it->second;
    }
    return &textures.emplace(t->serial, std::move(value)).first->second;
}
VkImageView textureView(Texture &t, const view_t *v, bool attachment = false) {
    view_t def{};
    const view_t &q = v ? *v : def;
    std::string key((const char *)&q, sizeof q);
    key += attachment ? 'a' : 's';
    auto old = t.views.find(key);
    if (old != t.views.end()) return old->second;
    VkImageViewCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ci.image = t.image;
    ci.format = t.format;
    if (q.set_format) {
        VkFormat f = format(q.format);
        if ((t.format == VK_FORMAT_R8G8B8A8_UNORM || t.format == VK_FORMAT_R8G8B8A8_SRGB) &&
            (f == VK_FORMAT_R8G8B8A8_UNORM || f == VK_FORMAT_R8G8B8A8_SRGB)) ci.format = f;
    }
    unsigned level = q.set_levels ? std::min(t.levels - 1, unsigned(std::max(0, q.base_level))) : 0;
    unsigned layer = q.set_layers ? std::min(t.layers - 1, unsigned(std::max(0, q.min_layer))) : 0;
    unsigned levels = q.set_levels && q.nlevels > 0 ? std::min(t.levels - level, unsigned(q.nlevels)) : t.levels - level;
    unsigned layers = q.set_layers && q.nlayers > 0 ? std::min(t.layers - layer, unsigned(q.nlayers)) : t.layers - layer;
    int target = q.set_target ? q.target : t.target;
    ci.viewType = target == 2 ? VK_IMAGE_VIEW_TYPE_3D : target == 8 ? VK_IMAGE_VIEW_TYPE_CUBE :
                  target == 9 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : target == 4 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    if (attachment) { ci.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; levels = 1; }
    VkImageAspectFlags aspect = attachment ? t.aspect : t.aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : t.aspect;
    ci.subresourceRange = {aspect, level, levels, layer, layers};
    VkImageView view{};
    check(vkCreateImageView(dev, &ci, nullptr, &view), "texture subview");
    t.views.emplace(key, view);
    return view;
}
Texture *texid(int id) {
    auto it = registrations.find(id);
    return it == registrations.end() ? nullptr : texture(&it->second.tex);
}
VkSampler sampler(int id) {
    auto it = samplers.find(id);
    if (it != samplers.end())
        return it->second;
    VkSamplerCreateInfo c{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    c.minFilter = c.magFilter = VK_FILTER_LINEAR;
    c.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    c.addressModeU = c.addressModeV = c.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    c.maxLod = 0;
    auto p = samplerParams.find(id);
    if (p != samplerParams.end()) {
        const auto &s = p->second;
        c.magFilter = s.magf ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        c.minFilter = (s.minf == 1 || s.minf == 3 || s.minf == 5) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        auto wrap = [](int w) {
            return w == 1 ? VK_SAMPLER_ADDRESS_MODE_REPEAT : w == 6 ? VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT : w == 5 ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        };
        c.addressModeU = wrap(s.ws);
        c.addressModeV = wrap(s.wt);
        c.addressModeW = wrap(s.wr);
        c.compareEnable = s.cmp_mode != 0;
        c.compareOp = VkCompareOp(s.cmp_func ? s.cmp_func - 1 : 0);
        c.minLod = std::max(0.f, s.lod_min);
        c.maxLod = std::max(c.minLod, std::min(16.f, s.lod_max));
        c.mipLodBias = std::max(-properties.limits.maxSamplerLodBias,
                               std::min(properties.limits.maxSamplerLodBias, s.lod_bias));
        c.mipmapMode = s.minf >= 4 ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
    VkSampler v{};
    check(vkCreateSampler(dev, &c, nullptr, &v), "sampler");
    samplers[id] = v;
    return v;
}
Shader *shader(const NVNshaderData &sd) {
    char key[17];
    if (!sd.control)
        return nullptr;
    dmNvnShaderKey(sd.control, key);
    auto it = shaders.find(key);
    if (it != shaders.end())
        return it->second.module ? &it->second : nullptr;
    Shader sh;
    sh.key = key;
    auto binary = read(std::string(base) + "shaders/" + key + ".spv");
    if (binary.size() < 20) {
        if (missing++ < 50)
            note("missing shader %s", key);
        shaders.emplace(key, sh);
        return nullptr;
    }
    auto *w = (uint32_t *)binary.data();
    size_t nw = binary.size() / 4;
    std::vector<uint32_t> bind(w[3], UINT32_MAX), type(w[3]), storage(w[3]), pointee(w[3]),
        loc(w[3], UINT32_MAX);
    std::vector<std::array<uint32_t, 3>> image(w[3]);
    for (size_t i = 5; i < nw;) {
        uint32_t n = w[i] >> 16, op = w[i] & 65535;
        if (!n || i + n > nw)
            break;
        if (op == 15 && n >= 3)
            sh.stage = w[i + 1] == 0 ? 0 : w[i + 1] == 4 ? 1 : 5;
        if (op == 71 && n >= 4) {
            if (w[i + 2] == 33)
                bind[w[i + 1]] = w[i + 3];
            if (w[i + 2] == 30)
                loc[w[i + 1]] = w[i + 3];
        }
        if (op == 32 && n >= 4)
            pointee[w[i + 1]] = w[i + 3];
        if (op == 25 && n >= 9) {
            type[w[i + 1]] = 25;
            image[w[i + 1]] = {w[i + 3], w[i + 7], w[i + 5]};
        }
        if (op == 26 && n >= 2)
            type[w[i + 1]] = 26;
        if (op == 27 && n >= 3) {
            type[w[i + 1]] = 27;
            pointee[w[i + 1]] = w[i + 2];
        }
        if (op == 59 && n >= 4) {
            type[w[i + 2]] = w[i + 1];
            storage[w[i + 2]] = w[i + 3];
        }
        i += n;
    }
    // Each translated stage owns its descriptor set. Binding numbers overlap by design.
    for (size_t i = 5; i < nw;) {
        uint32_t n = w[i] >> 16, op = w[i] & 65535;
        if (!n)
            break;
        if (op == 71 && n >= 4 && w[i + 2] == 34)
            w[i + 3] = sh.stage == 1 ? 1 : 0;
        i += n;
    }
    auto jr = read(std::string(base) + "shaders/" + key + ".res.json");
    if (jr.empty())
        jr = read(std::string(base) + "shaders/" + key + ".spv.res.json");
    std::string json(jr.begin(), jr.end());
    for (uint32_t id = 1; id < bind.size(); id++)
        if (bind[id] != UINT32_MAX) {
            Binding b;
            b.binding = bind[id];
            uint32_t t = pointee[type[id]];
            b.kind = 0;
            b.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            if (storage[id] == 12) {
                b.kind = 2;
                b.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            }
            if (storage[id] == 0) {
                if (type[t] == 27) {
                    b.kind = 3;
                    b.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    t = pointee[t];
                } else if (type[t] == 26) {
                    b.kind = 4;
                    b.type = VK_DESCRIPTOR_TYPE_SAMPLER;
                } else if (type[t] == 25) {
                    b.kind = image[t][1] == 2 ? 5 : 3;
                    b.type = image[t][1] == 2 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                                              : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                }
                b.dim = image[t][0];
                if (b.dim == 5)
                    b.type = b.kind == 5 ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
                                         : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            }
            for (auto &o : objects(json, "cbufs"))
                if (field(o, "binding", -1) == int(b.binding)) {
                    b.kind = 1;
                    b.slot = field(o, "slot", -1);
                }
            for (auto &o : objects(json, "sbufs"))
                if (field(o, "binding", -1) == int(b.binding)) {
                    b.kind = 2;
                    b.slot = field(o, "slot", -1);
                    b.cb = field(o, "cb_slot");
                    b.offset = field(o, "cb_offset") * 4;
                }
            for (auto &section : {"textures", "images"})
                for (auto &o : objects(json, section))
                    if (field(o, "binding", -1) == int(b.binding)) {
                        b.handle = field(o, "handle");
                        b.cbuf = field(o, "cbuf_slot", -1);
                        b.separate = field(o, "separate") != 0;
                        b.count = field(o, "array", 1);
                    }
            sh.bindings.push_back(b);
        }
    for (uint32_t id = 1; id < loc.size(); id++)
        if (storage[id] == 1 && loc[id] != UINT32_MAX)
            sh.inputs.push_back(loc[id]);
    std::sort(sh.inputs.begin(), sh.inputs.end());
    VkShaderModuleCreateInfo mc{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    mc.codeSize = binary.size();
    mc.pCode = w;
    check(vkCreateShaderModule(dev, &mc, nullptr, &sh.module), "shader module");
    std::vector<VkDescriptorSetLayoutBinding> layout;
    for (auto &b : sh.bindings)
        layout.push_back({b.binding, b.type, (uint32_t)b.count,
                          sh.stage == 0   ? VK_SHADER_STAGE_VERTEX_BIT
                          : sh.stage == 1 ? VK_SHADER_STAGE_FRAGMENT_BIT
                                          : VK_SHADER_STAGE_COMPUTE_BIT,
                          nullptr});
    VkDescriptorSetLayoutCreateInfo lc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lc.bindingCount = layout.size();
    lc.pBindings = layout.data();
    check(vkCreateDescriptorSetLayout(dev, &lc, nullptr, &sh.layout), "descriptor layout");
    note("shader %s stage=%d binds=%zu inputs=%zu", key, sh.stage, sh.bindings.size(), sh.inputs.size());
    Shader *result = &shaders.emplace(key, std::move(sh)).first->second;
    if (submits < 2)
        note("shader returned %p", (void *)result);
    return result;
}
Prog *program() {
    if (!S.prog)
        return nullptr;
    if (submits < 2)
        note("program resolve serial=%llu stages=%u", (unsigned long long)S.prog->serial, S.prog->nstage);
    auto it = programs.find(S.prog->serial);
    if (it != programs.end())
        return &it->second;
    Prog p;
    for (unsigned i = 0; i < S.prog->nstage; i++) {
        Shader *sh = shader(S.prog->sd[i]);
        if (sh)
            p.sh[sh->stage] = sh;
    }
    if (submits < 2)
        note("program stages=%u resolved cs=%p vs=%p ps=%p", S.prog->nstage, (void *)p.sh[5], (void *)p.sh[0],
             (void *)p.sh[1]);
    return &programs.emplace(S.prog->serial, p).first->second;
}
struct Pass {
    VkRenderPass pass{};
};
std::map<std::string, Pass> passes;
std::string passkey() {
    std::string k;
    for (int i = 0; i < 8; i++) {
        int f = i < S.rt.n && S.rt.color[i] ? format(S.rt.color[i]->d.format) : 0;
        k.append((char *)&f, 4);
    }
    int f = S.rt.depth ? format(S.rt.depth->d.format) : 0;
    k.append((char *)&f, 4);
    return k;
}
bool beginpass() {
    if (inPass)
        return true;
    std::vector<Texture *> targets;
    std::vector<VkImageView> views;
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colors;
    Texture *depth{};
    passW = passH = 0;
    for (int i = 0; i < S.rt.n; i++) {
        Texture *t = texture(S.rt.color[i]);
        colors.push_back({VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_GENERAL});
        if (!t)
            continue;
        colors.back().attachment = attachments.size();
        VkAttachmentDescription a{};
        a.format = t->format;
        a.samples = VK_SAMPLE_COUNT_1_BIT;
        a.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        a.initialLayout = a.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        attachments.push_back(a);
        targets.push_back(t);
        const view_t *view = S.rt.has_cview[i] ? &S.rt.cview[i] : nullptr;
        views.push_back(textureView(*t, view, true));
        unsigned level = view && view->set_levels ? std::max(0, view->base_level) : 0;
        unsigned tw = std::max(1u, t->w >> level), th = std::max(1u, t->h >> level);
        passW = passW ? std::min(passW, tw) : tw;
        passH = passH ? std::min(passH, th) : th;
    }
    VkAttachmentReference dr{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_GENERAL};
    if ((depth = texture(S.rt.depth))) {
        dr.attachment = attachments.size();
        VkAttachmentDescription a{};
        a.format = depth->format;
        a.samples = VK_SAMPLE_COUNT_1_BIT;
        a.loadOp = a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        a.storeOp = a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        a.initialLayout = a.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        attachments.push_back(a);
        targets.push_back(depth);
        views.push_back(textureView(*depth, S.rt.has_dview ? &S.rt.dview : nullptr, true));
        passW = passW ? std::min(passW, depth->w) : depth->w;
        passH = passH ? std::min(passH, depth->h) : depth->h;
    }
    if (targets.empty())
        return false;
    std::string key = passkey();
    auto it = passes.find(key);
    if (it == passes.end()) {
        VkSubpassDescription s{};
        s.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        s.colorAttachmentCount = colors.size();
        s.pColorAttachments = colors.data();
        s.pDepthStencilAttachment = depth ? &dr : nullptr;
        VkRenderPassCreateInfo pc{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        pc.attachmentCount = attachments.size();
        pc.pAttachments = attachments.data();
        pc.subpassCount = 1;
        pc.pSubpasses = &s;
        Pass p;
        if (vkCreateRenderPass(dev, &pc, nullptr, &p.pass))
            return false;
        it = passes.emplace(key, p).first;
    }
    currentPass = it->second.pass;
    VkFramebufferCreateInfo fc{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fc.renderPass = currentPass;
    fc.attachmentCount = views.size();
    fc.pAttachments = views.data();
    fc.width = passW;
    fc.height = passH;
    fc.layers = 1;
    VkFramebuffer fb{};
    if (vkCreateFramebuffer(dev, &fc, nullptr, &fb))
        return false;
    framebuffers.push_back(fb);
    VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rb.renderPass = currentPass;
    rb.framebuffer = fb;
    rb.renderArea = {{0, 0}, {passW, passH}};
    vkCmdBeginRenderPass(cmd, &rb, VK_SUBPASS_CONTENTS_INLINE);
    inPass = true;
    return true;
}
struct Pipeline {
    VkPipeline pipe{};
    VkPipelineLayout layout{};
};
std::map<std::string, Pipeline> pipelines;
VkBlendFactor blendFactor(int f) {
    switch (f) {
    case 1:
        return VK_BLEND_FACTOR_ZERO;
    case 2:
        return VK_BLEND_FACTOR_ONE;
    case 3:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case 4:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 5:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case 6:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 7:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case 8:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 9:
        return VK_BLEND_FACTOR_DST_COLOR;
    case 10:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case 11:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case 0x61:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case 0x62:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case 0x63:
        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case 0x64:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    default:
        return VK_BLEND_FACTOR_ONE;
    }
}
VkPrimitiveTopology topology(int p) {
    switch (p) {
    case 0:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case 1:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case 3:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case 5:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case 6:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}
VkCompareOp compare(int n) { return n >= 1 && n <= 8 ? (VkCompareOp)(n - 1) : VK_COMPARE_OP_ALWAYS; }
Pipeline *pipeline(Prog &p, int prim, bool compute = false) {
    Shader *vs = p.sh[compute ? 5 : 0], *ps = compute ? nullptr : p.sh[1];
    if (!vs)
        return nullptr;
    std::string key = vs->key + (ps ? ps->key : "");
    key.append((char *)&prim, sizeof prim);
    if (!compute) {
        key += passkey();
        key.append((char *)S.va, sizeof S.va);
        key.append((char *)S.streams, sizeof S.streams);
        key.append((char *)S.blend, sizeof S.blend);
        key.append((char *)&S.color, sizeof S.color);
        key.append((char *)&S.mask, sizeof S.mask);
        key.append((char *)&S.ds, sizeof S.ds);
        key.append((char *)&S.poly, sizeof S.poly);
        key.append((char *)&debugFlags, sizeof debugFlags);
    }
    auto it = pipelines.find(key);
    if (it != pipelines.end())
        return it->second.pipe ? &it->second : nullptr;
    Pipeline out;
    VkDescriptorSetLayout layouts[2]{vs->layout, ps ? ps->layout : VK_NULL_HANDLE};
    VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    lc.setLayoutCount = ps ? 2 : 1;
    lc.pSetLayouts = layouts;
    if (vkCreatePipelineLayout(dev, &lc, nullptr, &out.layout))
        return nullptr;
    VkPipelineShaderStageCreateInfo stages[2]{};
    for (int i = 0; i < (ps ? 2 : 1); i++) {
        stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[i].stage = compute ? VK_SHADER_STAGE_COMPUTE_BIT
                          : i     ? VK_SHADER_STAGE_FRAGMENT_BIT
                                  : VK_SHADER_STAGE_VERTEX_BIT;
        stages[i].module = i ? ps->module : vs->module;
        stages[i].pName = "main";
    }
    VkResult result;
    if (compute) {
        if (submits < 2)
            note("create compute %s", vs->key.c_str());
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage = stages[0];
        ci.layout = out.layout;
        result = vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, &out.pipe);
    } else {
        std::vector<VkVertexInputAttributeDescription> attrs;
        std::vector<VkVertexInputBindingDescription> bindings;
        std::array<bool, 16> seen{};
        for (uint32_t loc : vs->inputs) {
            if (loc >= 16)
                continue;
            va_st a = S.va[loc];
            uint32_t stream = a.stream;
            if (stream >= 16)
                continue;
            VkFormat f = a.fmt ? format(a.fmt) : VK_FORMAT_R32G32B32A32_SFLOAT;
            attrs.push_back({loc, stream, f, (uint32_t)a.offset});
            if (!seen[stream]) {
                bindings.push_back({stream, S.streams[stream].stride,
                                    S.streams[stream].divisor ? VK_VERTEX_INPUT_RATE_INSTANCE
                                                              : VK_VERTEX_INPUT_RATE_VERTEX});
                seen[stream] = true;
            }
        }
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vi.vertexBindingDescriptionCount = bindings.size();
        vi.pVertexBindingDescriptions = bindings.data();
        vi.vertexAttributeDescriptionCount = attrs.size();
        vi.pVertexAttributeDescriptions = attrs.data();
        VkPipelineInputAssemblyStateCreateInfo ia{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = topology(prim);
        ia.primitiveRestartEnable = prim == 5 || prim == 3;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount = vp.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = S.poly.cull == 1 ? VK_CULL_MODE_FRONT_BIT : S.poly.cull == 2 ? VK_CULL_MODE_BACK_BIT : S.poly.cull == 3 ? VK_CULL_MODE_FRONT_AND_BACK : VK_CULL_MODE_NONE;
        if (debugFlags & 4) rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = S.poly.front ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rs.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        std::vector<VkPipelineColorBlendAttachmentState> blend;
        for (int i = 0; i < S.rt.n; i++) {
            VkPipelineColorBlendAttachmentState c{};
            c.colorWriteMask = (S.mask.mask >> (i * 4)) & 15;
            c.blendEnable = (S.color.blend_en >> i) & 1;
            const auto &b = S.blend[i];
            c.srcColorBlendFactor = blendFactor(b.src_rgb);
            c.dstColorBlendFactor = blendFactor(b.dst_rgb);
            c.srcAlphaBlendFactor = blendFactor(b.src_a);
            c.dstAlphaBlendFactor = blendFactor(b.dst_a);
            c.colorBlendOp = (VkBlendOp)(b.eq_rgb ? b.eq_rgb - 1 : 0);
            c.alphaBlendOp = (VkBlendOp)(b.eq_a ? b.eq_a - 1 : 0);
            blend.push_back(c);
        }
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount = blend.size();
        cb.pAttachments = blend.data();
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable = S.rt.depth && S.ds.dt;
        ds.depthWriteEnable = S.rt.depth && S.ds.dw;
        ds.depthCompareOp = compare(S.ds.df);
        if ((debugFlags & 1) && S.ds.df == 3) ds.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
        if (debugFlags & 2) ds.depthTestEnable = false;
        ds.minDepthBounds = 0;
        ds.maxDepthBounds = 1;
        // Stencil enum values map from NVN's GL ordering to Vulkan.
        auto sop = [](unsigned v) {
            switch (v) {
            case 1:
                return VK_STENCIL_OP_KEEP;
            case 2:
                return VK_STENCIL_OP_ZERO;
            case 3:
                return VK_STENCIL_OP_REPLACE;
            case 4:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case 5:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case 6:
                return VK_STENCIL_OP_INVERT;
            case 7:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case 8:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            default:
                return VK_STENCIL_OP_KEEP;
            }
        };
        ds.stencilTestEnable = S.rt.depth && S.ds.st;
        ds.front = {sop(S.ds.fail_f), sop(S.ds.zpass_f), sop(S.ds.zfail_f), compare(S.ds.sf_f), 255, 255, 0};
        ds.back = {sop(S.ds.fail_b), sop(S.ds.zpass_b), sop(S.ds.zfail_b), compare(S.ds.sf_b), 255, 255, 0};
        VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR,
                                VK_DYNAMIC_STATE_BLEND_CONSTANTS,
                                VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                                VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
                                VK_DYNAMIC_STATE_STENCIL_WRITE_MASK};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 6;
        dynamic.pDynamicStates = dyn;
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        ci.stageCount = ps ? 2 : 1;
        ci.pStages = stages;
        ci.pVertexInputState = &vi;
        ci.pInputAssemblyState = &ia;
        ci.pViewportState = &vp;
        ci.pRasterizationState = &rs;
        ci.pMultisampleState = &ms;
        ci.pDepthStencilState = &ds;
        ci.pColorBlendState = &cb;
        ci.pDynamicState = &dynamic;
        ci.layout = out.layout;
        ci.renderPass = currentPass;
        result = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, &out.pipe);
    }
    if (result) {
        if (pipelineErrors++ < 50)
            note("pipeline %s %s primitive=%d error=%d", vs->key.c_str(), ps ? ps->key.c_str() : "CS", prim,
                 result);
    }
    return &pipelines.emplace(key, out).first->second;
}
uint64_t handleWord(int stage, int bank, int word) {
    size_t off = (size_t)word * 4;
    if (bank == 2) {
        if (off >= 0x168 && off < 0x568)
            return S.tex[stage][(off - 0x168) / 8];
        if (off >= 0x568 && off < 0x968)
            return S.samp[stage][(off - 0x568) / 8];
        if (off >= 0x120 && off < 0x168)
            return S.img[stage][(off - 0x120) / 8];
        return 0;
    }
    if (bank >= 3 && bank < 19) {
        auto u = S.ub[stage][bank - 3];
        uint32_t v{};
        if (u.addr && off + 4 <= u.size)
            memcpy(&v, (void *)(uintptr_t)(u.addr + off), 4);
        return v;
    }
    return 0;
}
uint64_t bindingHandle(const Binding &b, int stage) {
    uint32_t h = b.handle;
    int tb = b.cbuf < 0 ? 2 : b.cbuf & 65535;
    int sb = b.cbuf < 0 ? 2 : (b.cbuf >> 16) ? (b.cbuf >> 16) - 1 : tb;
    int kind = h >> 28;
    uint64_t t = handleWord(stage, tb, h & 0x3fff);
    if (kind) {
        uint64_t s = handleWord(stage, sb, (h >> 14) & 0x3fff);
        if (kind == 3) s = ((h >> 14) & 0x3fff) << 20;
        else if (kind == 2) s <<= 20;
        return t | s;
    }
    return t;
}
Texture *dummyTexture(int dim) {
    tex_t t{};
    t.serial = UINT64_MAX - dim;
    t.d.width = t.d.height = t.d.depth = t.d.levels = 1;
    t.d.target = dim == 2 ? 2 : dim == 3 ? 8 : 1;
    t.d.format = 0x25;
    return texture(&t);
}

Slice bindingBuffer(const Binding &b, Shader &sh) {
    int stage = sh.stage;
    if (b.kind == 0) {
        if (supportSlice.b)
            return supportSlice;
        std::array<uint8_t, 65536> support{};
        for (int i = 0; i < 73; i++)
            *(float *)(support.data() + 192 + 16 * i) = 1;
        return supportSlice = bytes(support.data(), support.size());
    }
    if (b.kind == 1) {
        if (b.slot >= 3 && b.slot < 19) {
            auto &u = S.ub[stage][b.slot - 3];
            return guest(u.addr, u.size);
        }
        if (b.slot == 1) {
            auto it = constantSlices.find(sh.key);
            if (it != constantSlices.end())
                return it->second;
        }
        std::array<uint8_t, 65536> driver{};
        if (b.slot == 2) {
            memcpy(driver.data() + 0x168, S.tex[stage], sizeof S.tex[stage]);
            memcpy(driver.data() + 0x568, S.samp[stage], sizeof S.samp[stage]);
            memcpy(driver.data() + 0x120, S.img[stage], 8 * 8);
        }
        if (b.slot == 0) {
            int offset = stage == 0 ? 0x110 : stage == 1 ? 0x510 : 0x310;
            for (int i = 0; i < 16; i++) {
                memcpy(driver.data() + offset + 16 * i, &S.ssbo[stage][i].addr, 8);
                uint32_t sz = S.ssbo[stage][i].size;
                memcpy(driver.data() + offset + 16 * i + 8, &sz, 4);
            }
        }
        if (submits < 2)
            note("driver bank %d stage=%d", b.slot, stage);
        if (b.slot == 1) {
            auto it = shaderConstants.find(sh.key);
            if (it != shaderConstants.end())
                memcpy(driver.data(), it->second.data(), std::min(driver.size(), it->second.size()));
        }
        Slice result = bytes(driver.data(), b.slot == 1 ? driver.size() : 4096);
        if (b.slot == 1)
            constantSlices[sh.key] = result;
        return result;
    }
    if (b.kind == 2) {
        int baseoff = stage == 0 ? 0x110 : stage == 1 ? 0x510 : 0x310;
        int slot = (b.offset - baseoff) / 16;
        p_bind_buf u{};
        if (stage == 5 && b.offset >= 0x210 && b.offset < 0x310)
            u = S.ub[stage][(b.offset - 0x210) / 16];
        else if (slot >= 0 && slot < 16)
            u = S.ssbo[stage][slot];
        if (submits < 8)
            note("storage binding=%u stage=%d cbOffset=%x slot=%d addr=%llx size=%llu", b.binding, stage,
                 b.offset, slot, (unsigned long long)u.addr, (unsigned long long)u.size);
        Slice s = guest(u.addr, u.size);
        if (u.addr && u.size)
            writebacks.push_back({u.addr, s});
        return s;
    }
    return zeroSlice.b ? zeroSlice : (zeroSlice = bytes(nullptr, 65536));
}
VkDescriptorSet descriptors(Shader &sh) {
    if (submits < 2)
        note("allocate descriptors %s layout=%p pool=%p", sh.key.c_str(), (void *)sh.layout,
             (void *)descpool);
    VkDescriptorSet set{};
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = descpool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &sh.layout;
    if (vkAllocateDescriptorSets(dev, &ai, &set))
        return {};
    for (auto &b : sh.bindings) {
        if (submits < 2)
            note("descriptor %s binding=%u kind=%d type=%d slot=%d", sh.key.c_str(), b.binding, b.kind,
                 b.type, b.slot);
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = set;
        w.dstBinding = b.binding;
        w.descriptorCount = b.count;
        w.descriptorType = b.type;
        std::vector<VkDescriptorBufferInfo> bs(b.count);
        std::vector<VkDescriptorImageInfo> is(b.count);
        std::vector<VkBufferView> views(b.count);
        if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || b.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            Slice s = bindingBuffer(b, sh);
            for (auto &v : bs)
                v = {s.b, s.off,
                     std::min(s.size, b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                          ? size_t(properties.limits.maxUniformBufferRange)
                                          : size_t(properties.limits.maxStorageBufferRange))};
            w.pBufferInfo = bs.data();
        } else if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
                   b.type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            uint64_t h = bindingHandle(b, sh.stage);
            auto it = registrations.find(h & 0xfffff);
            Slice slice;
            VkFormat fmt = VK_FORMAT_R32G32B32A32_SFLOAT;
            if (it != registrations.end() && it->second.tex.mem) {
                auto &t = it->second.tex;
                slice = guest((uint64_t)(uintptr_t)t.mem, t.L.size);
                fmt = format(t.d.format);
            } else
                slice = bytes(nullptr, 4096);
            VkBufferViewCreateInfo vi{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
            vi.buffer = slice.b;
            vi.format = fmt;
            vi.offset = slice.off;
            vi.range = slice.size;
            VkBufferView view{};
            if (vkCreateBufferView(dev, &vi, nullptr, &view))
                return {};
            bufferViews.push_back(view);
            std::fill(views.begin(), views.end(), view);
            w.pTexelBufferView = views.data();
        } else {
            uint64_t h = bindingHandle(b, sh.stage);
            Texture *t = b.kind == 4 ? nullptr : texid(h & 0xfffff);
            if (!sh.logged)
                note("image binding=%u stage=%d kind=%d dim=%d tid=%u sid=%u view=%p fmt=%d shape=%ux%ux%u",
                     b.binding, sh.stage, b.kind, b.dim, (unsigned)(h & 0xfffff), (unsigned)(h >> 20),
                     t ? (void *)t->view : nullptr, t ? t->format : 0, t ? t->w : 0, t ? t->h : 0,
                     t ? t->depth : 0);
            if (!t && b.kind != 4)
                t = dummyTexture(b.dim);
            if (!t && b.kind != 4)
                return {};
            const view_t *view = nullptr;
            auto reg = registrations.find(h & 0xfffff);
            if (reg != registrations.end() && reg->second.hasView) view = &reg->second.view;
            VkImageView iv = t ? textureView(*t, view) : VK_NULL_HANDLE;
            for (auto &v : is)
                v = {sampler(h >> 20), iv, VK_IMAGE_LAYOUT_GENERAL};
            w.pImageInfo = is.data();
        }
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
    }
    sh.logged = true;
    return set;
}
void dynamic() {
    float w = S.viewport[2] > 0 ? S.viewport[2] : passW, h = S.viewport[3] > 0 ? S.viewport[3] : passH;
    VkViewport vp{S.viewport[0], S.viewport[1] + h, w, -h, S.nearZ, S.farZ};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    int x = std::max(0, S.sc.x), y = std::max(0, S.sc.y);
    uint32_t sw = S.sc.w > 0 ? std::min(uint32_t(S.sc.w), passW - std::min(passW, uint32_t(x))) : passW,
             sh = S.sc.h > 0 ? std::min(uint32_t(S.sc.h), passH - std::min(passH, uint32_t(y))) : passH;
    VkRect2D sc{{x, y}, {sw, sh}};
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdSetBlendConstants(cmd, S.blendColor);
    vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilRef);
    vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilValueMask);
    vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilMask);
}
void draw(const p_draw *dw, const p_draw_idx *di) {
    Prog *p = program();
    if (!p || !p->sh[0])
        return;
    lastGraphics = p->sh[0]->key + "/" + (p->sh[1] ? p->sh[1]->key : "depth");
    // Resource creation and transfer commands must precede the render pass.
    VkDescriptorSet sets[2]{descriptors(*p->sh[0]), p->sh[1] ? descriptors(*p->sh[1]) : VK_NULL_HANDLE};
    if (!sets[0] || (p->sh[1] && !sets[1]) || !beginpass())
        return;
    Pipeline *pipe = pipeline(*p, di ? di->prim : dw->prim);
    if (!pipe || !pipe->pipe)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, p->sh[1] ? 2 : 1, sets, 0, nullptr);
    dynamic();
    if (draws < 4) {
        note("draw %llu vs=%s ps=%s rt=%dx%d viewport=%g,%g,%g,%g count=%d first=%d",
             (unsigned long long)draws, p->sh[0]->key.c_str(), p->sh[1] ? p->sh[1]->key.c_str() : "depth", passW, passH,
             S.viewport[0], S.viewport[1], S.viewport[2], S.viewport[3], di ? di->count : dw->count,
             di ? di->base_vertex : dw->first);
        for (int i = 0; i < 4; i++) {
            auto a = S.va[i];
            note(" attr%d fmt=%u off=%u stream=%u stride=%u vb=%llx size=%llu", i, a.fmt, a.offset, a.stream,
                 S.streams[a.stream].stride, (unsigned long long)S.vb[a.stream].address,
                 (unsigned long long)S.vb[a.stream].size);
        }
        auto u = S.ub[0][0];
        if (u.addr && u.size >= 64) {
            float f[16];
            memcpy(f, (void *)(uintptr_t)u.addr, 64);
            note(" matrix cb0 %g %g %g %g | %g %g %g %g | %g %g %g %g | %g %g %g %g", f[0], f[1], f[2], f[3],
                 f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15]);
        }
    }
    if (draws < 4 && S.vb[0].address) {
        float v[20]{};
        memcpy(v, (void *)(uintptr_t)S.vb[0].address, std::min<uint64_t>(80, S.vb[0].size));
        note(" vertices %g %g %g %g %g | %g %g %g %g %g | %g %g %g %g %g | %g %g %g %g %g", v[0], v[1], v[2],
             v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15], v[16], v[17],
             v[18], v[19]);
    }
    if (!p->sh[0]->drawLogged) {
        p->sh[0]->drawLogged = true;
        note("model vs=%s ps=%s rt=%ux%u ds=%d/%d/%d poly=%d/%d count=%d base=%d", p->sh[0]->key.c_str(), p->sh[1] ? p->sh[1]->key.c_str() : "depth",passW,passH,int(S.ds.dt),int(S.ds.dw),int(S.ds.df),int(S.poly.cull),int(S.poly.front),di?di->count:dw->count,di?di->base_vertex:dw->first);
        for (auto loc : p->sh[0]->inputs) if(loc<16) {
            auto a = S.va[loc]; auto vb = S.vb[a.stream];
            note("  loc%u fmt=%u offset=%u stream=%u stride=%u div=%u vb=%llx size=%llu",loc,a.fmt,a.offset,a.stream,S.streams[a.stream].stride,S.streams[a.stream].divisor,(unsigned long long)vb.address,(unsigned long long)vb.size);
        }
        auto save = [&](const std::string &name, uint64_t ptr, size_t n) {
            if (!ptr || !n) return;
            FILE *f = fopen((std::string(base)+"model_"+p->sh[0]->key+"_"+name+".bin").c_str(),"wb");
            if(f) { fwrite((const void *)(uintptr_t)ptr,1,std::min(n,size_t(2<<20)),f);fclose(f); }
        };
        save("vb",S.vb[0].address,S.vb[0].size);
        if(di) save("ib",di->ibuf,di->count*(di->itype==2?4:di->itype==1?2:1));
        for(int i=0;i<16;i++) save("cb"+std::to_string(i),S.ub[0][i].addr,S.ub[0][i].size);
    }
    uint32_t maxIndex = dw ? dw->first + dw->count : 0;
    Slice indices{};
    VkIndexType it = VK_INDEX_TYPE_UINT16;
    if (di) {
        size_t unit = di->itype == 2 ? 4 : di->itype == 1 ? 2 : 1;
        const uint8_t *ptr = (const uint8_t *)(uintptr_t)di->ibuf;
        if (!ptr || di->count <= 0 || di->count > 10000000)
            return;
        for (int i = 0; i < di->count; i++) {
            uint32_t v = 0;
            memcpy(&v, ptr + i * unit, unit);
            if ((unit == 2 && v == 65535) || (unit == 4 && v == UINT32_MAX))
                continue;
            maxIndex = std::max(maxIndex, v + 1);
        }
        if (di->itype == 0) {
            std::vector<uint16_t> expanded(di->count);
            for (int i = 0; i < di->count; i++)
                expanded[i] = ptr[i];
            indices = bytes(expanded.data(), expanded.size() * 2);
        } else
            indices = guest(di->ibuf, di->count * unit);
        it = di->itype == 2 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    }
    std::array<bool, 16> bound{};
    for (uint32_t loc : p->sh[0]->inputs) {
        if (loc >= 16)
            continue;
        int stream = S.va[loc].stream;
        if (bound[stream])
            continue;
        bound[stream] = true;
        auto v = S.vb[stream];
        uint64_t needed =
            (uint64_t)(maxIndex + std::max(di ? di->base_vertex : 0, 0)) * S.streams[stream].stride + 256;
        if (S.streams[stream].divisor)
            needed = (uint64_t)(di ? di->instances : 1) * S.streams[stream].stride + 256;
        size_t n = std::min<uint64_t>(v.size, needed);
        Slice b = v.address && n ? guest(v.address, n) : bytes(nullptr, 4096);
        VkDeviceSize off = b.off;
        vkCmdBindVertexBuffers(cmd, stream, 1, &b.b, &off);
    }
    if (di) {
        vkCmdBindIndexBuffer(cmd, indices.b, indices.off, it);
        vkCmdDrawIndexed(cmd, di->count, std::max(1, di->instances), 0, di->base_vertex, di->base_instance);
    } else if (dw->count > 0)
        vkCmdDraw(cmd, dw->count, 1, dw->first, 0);
    draws++;
}
bool builtinDispatch() {
    if (!S.prog || S.prog->nstage != 1)
        return false;
    char key[17];
    dmNvnShaderKey(S.prog->sd[0].control, key);
    int mode = !strcmp(key, "d0f769bb916bbba9")   ? 0
               : !strcmp(key, "16479cc6d75c2b28") ? 1
               : !strcmp(key, "0687e2ef95133577") ? 2
               : !strcmp(key, "4df5909e6dabee05") ? 3
                                                  : -1;
    if (mode < 0)
        return false;
    auto u = S.ub[5][0], src = S.ssbo[5][0], dst = S.ssbo[5][1];
    if (!u.addr || u.size < 4 || !src.addr || !dst.addr)
        return true;
    uint32_t n;
    memcpy(&n, (void *)(uintptr_t)u.addr, 4);
    n = std::min<uint64_t>(n, dst.size / 8);
    for (unsigned i = 0; i < n; i++) {
        uint64_t a = 0, b = 0;
        size_t off = i * (mode < 2 ? 16 : 32) + (mode % 2 ? 8 : 0);
        if (off + 8 > src.size)
            break;
        memcpy(&a, (void *)(uintptr_t)(src.addr + off), 8);
        if (mode >= 2 && off + 24 <= src.size)
            memcpy(&b, (void *)(uintptr_t)(src.addr + off + 16), 8);
        uint64_t v = mode >= 2 ? b - a : a;
        memcpy((void *)(uintptr_t)(dst.addr + i * 8), &v, 8);
    }
    copies.clear();
    return true;
}
void dispatch(const p_dispatch &d) {
    if (builtinDispatch())
        return;
    if (submits < 2)
        note("dispatch begin %dx%dx%d", d.x, d.y, d.z);
    endpass();
    Prog *p = program();
    if (!p || !p->sh[5])
        return;
    Shader &sh = *p->sh[5];
    lastCompute = sh.key;
    VkDescriptorSet set = descriptors(sh);
    Pipeline *pipe = pipeline(*p, 0, true);
    if (!set || !pipe || !pipe->pipe)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->layout, 0, 1, &set, 0, nullptr);
    vkCmdDispatch(cmd, d.x, d.y, d.z);
    dispatches++;
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1,
                         &mb, 0, nullptr, 0, nullptr);
}
void clearColor(const p_clear_color &c) {
    if (!beginpass() || c.index < 0 || c.index >= S.rt.n)
        return;
    VkClearAttachment a{};
    a.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    a.colorAttachment = c.index;
    memcpy(a.clearValue.color.float32, c.rgba, 16);
    VkClearRect r{{{0, 0}, {passW, passH}}, 0, 1};
    vkCmdClearAttachments(cmd, 1, &a, 1, &r);
}
void clearDS(const p_clear_ds &c) {
    if (!S.rt.depth || !beginpass())
        return;
    VkClearAttachment a{};
    a.aspectMask =
        (c.depth_mask ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | (c.stencil_mask ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    a.clearValue.depthStencil = {c.depth, (uint32_t)c.stencil};
    VkClearRect r{{{0, 0}, {passW, passH}}, 0, 1};
    if (a.aspectMask)
        vkCmdClearAttachments(cmd, 1, &a, 1, &r);
}
void copyB2T(const p_copy_b2t &c) {
    endpass();
    Texture *t = texture(c.tex);
    if (!t || !c.src)
        return;
    int bw, bh, bpp = nvn_fmt_bpb(c.tex->d.format, &bw, &bh);
    if (!bpp)
        return;
    size_t row = S.rowStride ? S.rowStride : (c.r.width + bw - 1) / bw * bpp;
    size_t size = row * ((c.r.height + bh - 1) / bh) * std::max(1, c.r.depth);
    Slice s = guest(c.src, size);
    VkBufferImageCopy copy{};
    copy.bufferOffset = s.off;
    copy.bufferRowLength = S.rowStride ? row / bpp * bw : 0;
    copy.imageSubresource = {t->aspect, 0, 0, 1};
    copy.imageOffset = {c.r.xoffset, c.r.yoffset, c.r.zoffset};
    copy.imageExtent = {(uint32_t)c.r.width, (uint32_t)c.r.height, (uint32_t)std::max(1, c.r.depth)};
    vkCmdCopyBufferToImage(cmd, s.b, t->image, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
}
void begin();
void finish(VkSemaphore wait = {}, VkSemaphore signal = {});
void execute(const uint8_t *p, int depth = 0) {
    if (depth > 32)
        return;
    for (unsigned count = 0; p && count < 1000000; count++) {
        if (deviceLost)
            return;
        op_hdr h;
        memcpy(&h, p, 8);
        const void *a = p + 8;
        if (h.op == OP_END)
            return;
        if (h.size < 8 || h.size > (64 << 20)) {
            note("invalid record op=%u bytes=%u", h.op, h.size);
            return;
        }
        switch (h.op) {
        case OP_JUMP:
            memcpy(&p, a, 8);
            continue;
        case OP_CALL: {
            const uint8_t *q;
            memcpy(&q, a, 8);
            execute(q, depth + 1);
            break;
        }
        case OP_SET_RT:
            endpass();
            S.rt = *(const p_set_rt *)a;
            break;
        case OP_CLEAR_COLOR:
            clearColor(*(const p_clear_color *)a);
            break;
        case OP_CLEAR_DS:
            clearDS(*(const p_clear_ds *)a);
            break;
        case OP_BIND_PROGRAM:
            S.prog = ((const p_bind_program *)a)->prog;
            break;
        case OP_BIND_STATE: {
            const auto &b = *(const p_bind_state *)a;
            switch (b.kind) {
            case 0: {
                auto s = *(const blend_st *)b.bytes;
                S.blend[s.target] = s;
                break;
            }
            case 1:
                memcpy(&S.color, b.bytes, sizeof S.color);
                break;
            case 2:
                memcpy(&S.mask, b.bytes, sizeof S.mask);
                break;
            case 4:
                memcpy(&S.poly, b.bytes, sizeof S.poly);
                break;
            case 5:
                memcpy(&S.ds, b.bytes, sizeof S.ds);
                break;
            }
            break;
        }
        case OP_BIND_VATTR: {
            const auto &b = *(const p_bind_vattr *)a;
            memcpy(S.va + b.first, b.st, b.n * 4);
            break;
        }
        case OP_BIND_VSTREAM: {
            const auto &b = *(const p_bind_vstream *)a;
            memcpy(S.streams + b.first, b.st, b.n * 8);
            break;
        }
        case OP_BIND_VBUF: {
            const auto &b = *(const p_bind_vbuf *)a;
            memcpy(S.vb + b.first, b.r, b.n * sizeof(NVNbufferRange));
            break;
        }
        case OP_BIND_UBO:
        case OP_BIND_SSBO: {
            const auto &b = *(const p_bind_buf *)a;
            if (b.stage >= 0 && b.stage < 6 && b.index >= 0 && b.index < 16)
                (h.op == OP_BIND_UBO ? S.ub[b.stage][b.index] : S.ssbo[b.stage][b.index]) = b;
            break;
        }
        case OP_BIND_TEX:
        case OP_BIND_SAMP:
        case OP_BIND_IMG: {
            const auto &b = *(const p_bind_handles *)a;
            if (b.stage >= 0 && b.stage < 6 && b.first >= 0 &&
                b.first + b.n <= (h.op == OP_BIND_IMG ? 32 : 128))
                memcpy((h.op == OP_BIND_TEX    ? S.tex[b.stage]
                        : h.op == OP_BIND_SAMP ? S.samp[b.stage]
                                               : S.img[b.stage]) +
                           b.first,
                       b.h, b.n * 8);
            break;
        }
        case OP_VIEWPORTS: {
            const auto &v = *(const p_viewports *)a;
            if (v.n && v.first == 0)
                memcpy(S.viewport, v.v[0], 16);
            break;
        }
        case OP_SCISSOR:
            S.sc = *(const p_scissor *)a;
            break;
        case OP_DEPTH_RANGE:
            S.nearZ = ((const p_depth_range *)a)->n;
            S.farZ = ((const p_depth_range *)a)->f;
            break;
        case OP_DRAW:
            draw((const p_draw *)a, nullptr);
            break;
        case OP_DRAW_IDX:
            draw(nullptr, (const p_draw_idx *)a);
            break;
        case OP_DISPATCH:
            dispatch(*(const p_dispatch *)a);
            break;
        case OP_BARRIER:
            endpass();
            break;
        case OP_UPDATE_UB: {
            uint64_t address, n;
            memcpy(&address, a, 8);
            memcpy(&n, (const uint8_t *)a + 8, 8);
            memcpy((void *)(uintptr_t)address, (const uint8_t *)a + 16, n);
            copies.clear();
            break;
        }
        case OP_COPY_B2B: {
            uint64_t x[3];
            memcpy(x, a, 24);
            memmove((void *)(uintptr_t)x[1], (void *)(uintptr_t)x[0], x[2]);
            copies.clear();
            break;
        }
        case OP_COPY_B2T:
            copyB2T(*(const p_copy_b2t *)a);
            break;
        case OP_COPY_T2T: {
            endpass();
            const auto &c = *(const p_copy_t2t *)a;
            Texture *s = texture(c.src), *d = texture(c.dst);
            if (s && d) {
                VkImageBlit b{};
                b.srcSubresource = {s->aspect, 0, 0, 1};
                b.dstSubresource = {d->aspect, 0, 0, 1};
                b.srcOffsets[0] = {c.sr.xoffset, c.sr.yoffset, c.sr.zoffset};
                b.srcOffsets[1] = {c.sr.xoffset + c.sr.width, c.sr.yoffset + c.sr.height,
                                   c.sr.zoffset + std::max(1, c.sr.depth)};
                b.dstOffsets[0] = {c.dr.xoffset, c.dr.yoffset, c.dr.zoffset};
                b.dstOffsets[1] = {c.dr.xoffset + c.dr.width, c.dr.yoffset + c.dr.height,
                                   c.dr.zoffset + std::max(1, c.dr.depth)};
                vkCmdBlitImage(cmd, s->image, VK_IMAGE_LAYOUT_GENERAL, d->image, VK_IMAGE_LAYOUT_GENERAL, 1,
                               &b, VK_FILTER_NEAREST);
            }
            break;
        }
        case OP_COPY_STRIDE:
            S.rowStride = ((const p_copy_stride *)a)->stride;
            break;
        case OP_COPY_IMAGE_STRIDE:
            S.imageStride = ((const p_copy_stride *)a)->stride;
            break;
        case OP_BLEND_COLOR:
            memcpy(S.blendColor, ((const p_blend_color *)a)->rgba, 16);
            break;
        case OP_STENCIL: {
            const auto &c = *(const p_stencil *)a;
            if (c.which == 0)
                S.stencilValueMask = c.value_mask;
            else if (c.which == 1)
                S.stencilMask = c.mask;
            else
                S.stencilRef = c.ref;
            break;
        }
        case OP_REPORT_COUNTER: {
            uint64_t x[2];
            memcpy(x, a, 16);
            uint64_t values[2]{0, serial.load()};
            memcpy((void *)(uintptr_t)x[1], values, 16);
            break;
        }
        case OP_SIGNAL_EVENT: {
            uint32_t *v;
            uint32_t mode, value;
            memcpy(&v, a, 8);
            memcpy(&mode, (const uint8_t *)a + 8, 4);
            memcpy(&value, (const uint8_t *)a + 12, 4);
            if (v) {
                if (mode == 1)
                    *v += value;
                else
                    *v = value;
            }
            break;
        }
        default:
            break;
        }
        p += h.size;
    }
}
void begin() {
    for (auto it = retiredTextures.begin(); it != retiredTextures.end() && it->first <= serial.load();) {
        auto t = textures.find(it->second);
        if (t != textures.end()) {
            for (auto &v : t->second.views) vkDestroyImageView(dev, v.second, nullptr);
            vkDestroyImageView(dev, t->second.view, nullptr);
            vkDestroyImage(dev, t->second.image, nullptr);
            vkFreeMemory(dev, t->second.memory, nullptr);
            textures.erase(t);
        }
        it = retiredTextures.erase(it);
    }
    arenaPage = 0;
    arena = arenaPages[0];
    cursor = 0;
    zeroSlice = {};
    supportSlice = {};
    constantSlices.clear();
    copies.clear();
    writebacks.clear();
    framebuffers.clear();
    vkResetCommandPool(dev, cmdpool, 0);
    vkResetDescriptorPool(dev, descpool, 0);
    VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    b.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd, &b), "begin command");
}
void finish(VkSemaphore wait, VkSemaphore signal) {
    endpass();
    check(vkEndCommandBuffer(cmd), "end command");
    VkSubmitInfo s{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    s.commandBufferCount = 1;
    s.pCommandBuffers = &cmd;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    if (wait) {
        s.waitSemaphoreCount = 1;
        s.pWaitSemaphores = &wait;
        s.pWaitDstStageMask = &stage;
    }
    if (signal) {
        s.signalSemaphoreCount = 1;
        s.pSignalSemaphores = &signal;
    }
    check(vkQueueSubmit(queue, 1, &s, VK_NULL_HANDLE), "submit");
    check(vkQueueWaitIdle(queue), "wait GPU");
    for (auto &t : replacedTextures) destroyTexture(t);
    replacedTextures.clear();
    for (auto &w : writebacks)
        if (w.slice.b)
            memcpy((void *)(uintptr_t)w.addr, w.slice.mapped, w.slice.size);
    for (auto fb : framebuffers)
        vkDestroyFramebuffer(dev, fb, nullptr);
    framebuffers.clear();
    for (auto v : bufferViews)
        vkDestroyBufferView(dev, v, nullptr);
    bufferViews.clear();
}
void stats() {
    FILE *f = fopen((std::string(base) + "dm_replay_stats.txt").c_str(), "w");
    if (f) {
        fprintf(f,
                "submit=%llu present=%llu draws=%llu dispatch=%llu upload=%llu shaders=%zu pipelines=%zu "
                "pipeline_errors=%llu missing_shader=%llu textures=%zu arena=%zu\n",
                (unsigned long long)submits, (unsigned long long)frames.load(), (unsigned long long)draws,
                (unsigned long long)dispatches, (unsigned long long)uploads, shaders.size(), pipelines.size(),
                (unsigned long long)pipelineErrors, (unsigned long long)missing, textures.size(), cursor);
        fclose(f);
    }
}
} // namespace
extern "C" uint64_t dmReplayFrames() { return frames.load(); }
extern "C" uint64_t dmReplaySerial() { return serial.load(); }
extern "C" void dmReplaySubmit(int n, const uint64_t *h) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!ready || deviceLost)
        return;
    begin();
    for (int i = 0; i < n; i++)
        execute((const uint8_t *)(uintptr_t)h[i]);
    finish();
    serial++;
    submits++;
    if (submits % 30 == 0)
        stats();
}
extern "C" void dmReplayPresent(const tex_t *t, const int *crop) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!ready || !t || deviceLost)
        return;
    uint32_t index;
    VkResult r = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &index);
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        check(r, "acquire");
        return;
    }
    if (FILE *config = fopen((std::string(base)+"dm_render_flags.txt").c_str(),"r")) {
        unsigned flags = 0; if (fscanf(config,"%u",&flags)==1) debugFlags=flags; fclose(config);
    } else debugFlags=0;
    begin();
    Texture *src = texture(t);
    struct Dump { int id; Texture *texture; Slice data; };
    std::vector<Dump> dumps;
    const std::string requestPath = std::string(base) + "dm_capture.txt";
    if (FILE *request = fopen(requestPath.c_str(), "r")) {
        int id;
        while (fscanf(request, "%d", &id) == 1) {
            Texture *image = texid(id);
            if (!image) continue;
            unsigned bpp = image->format == VK_FORMAT_R16G16B16A16_SFLOAT ? 8 : image->format == VK_FORMAT_R16G16_SFLOAT ? 4 : image->format == VK_FORMAT_R8_UNORM ? 1 : 4;
            Slice data = bytes(nullptr, (size_t)image->w * image->h * image->depth * bpp);
            VkBufferImageCopy copy{};
            copy.bufferOffset = data.off;
            copy.imageSubresource = {image->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : image->aspect,0,0,1};
            copy.imageExtent = {image->w,image->h,image->depth};
            imageBarrier(*image,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_GENERAL);
            vkCmdCopyImageToBuffer(cmd,image->image,VK_IMAGE_LAYOUT_GENERAL,data.b,1,&copy);
            dumps.push_back({id,image,data});
            auto reg = registrations.find(id);
            if (reg != registrations.end() && reg->second.tex.mem && reg->second.tex.has_layout) {
                const tex_t &guest = reg->second.tex;
                FILE *f = fopen((std::string(base)+"capture_"+std::to_string(id)+".guest.bin").c_str(),"wb");
                if (f) { fwrite(&guest.d,1,sizeof guest.d,f);fwrite(guest.mem,1,guest.L.size,f);fclose(f); }
            }
        }
        fclose(request); remove(requestPath.c_str());
    }
    Slice capture{};
    if (src && (frames == 1 || frames == 60 || frames == 1800) && src->aspect == VK_IMAGE_ASPECT_COLOR_BIT &&
        (src->format == VK_FORMAT_R8G8B8A8_UNORM || src->format == VK_FORMAT_R8G8B8A8_SRGB)) {
        capture = bytes(nullptr, (size_t)src->w * src->h * 4);
        VkBufferImageCopy cp{};
        cp.bufferOffset = capture.off;
        cp.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        cp.imageExtent = {src->w, src->h, 1};
        vkCmdCopyImageToBuffer(cmd, src->image, VK_IMAGE_LAYOUT_GENERAL, capture.b, 1, &cp);
    }
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(dev, swapchain, &count, nullptr);
    std::vector<VkImage> images(count);
    vkGetSwapchainImagesKHR(dev, swapchain, &count, images.data());
    Texture target;
    target.image = images[index];
    target.layers = target.levels = 1;
    imageBarrier(target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (src) {
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        int x = crop && crop[2] > 0 ? std::max(0, crop[0]) : 0;
        int y = crop && crop[3] > 0 ? std::max(0, crop[1]) : 0;
        int w = crop && crop[2] > 0 ? crop[2] : src->w;
        int h = crop && crop[3] > 0 ? crop[3] : src->h;
        blit.srcOffsets[0] = {x, y, 0};
        blit.srcOffsets[1] = {std::min(int(src->w), x + w), std::min(int(src->h), y + h), 1};
        blit.dstOffsets[1] = {(int)extent.width, (int)extent.height, 1};
        imageBarrier(*src, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
        vkCmdBlitImage(cmd, src->image, VK_IMAGE_LAYOUT_GENERAL, target.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
    } else {
        VkClearColorValue c{};
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(cmd, target.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &c, 1, &range);
    }
    imageBarrier(target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    finish(acquired, rendered);
    for (auto &dump : dumps) {
        FILE *f = fopen((std::string(base)+"capture_"+std::to_string(dump.id)+".bin").c_str(),"wb");
        if(f) {
            uint32_t header[]{dump.texture->w,dump.texture->h,dump.texture->depth,(uint32_t)dump.texture->format};
            fwrite(header,1,sizeof header,f); fwrite(dump.data.mapped,1,dump.data.size,f);fclose(f);
        }
    }
    if (capture.b) {
        FILE *f = fopen((std::string(base) + "dm_present.ppm").c_str(), "wb");
        size_t nonzero = 0;
        if (f) {
            fprintf(f, "P6\n%u %u\n255\n", src->w, src->h);
            std::vector<uint8_t> rgb((size_t)src->w * src->h * 3);
            for (size_t i = 0; i < (size_t)src->w * src->h; i++) {
                const uint8_t *p = capture.mapped + i * 4;
                memcpy(rgb.data() + i * 3, p, 3);
                nonzero += (p[0] || p[1] || p[2]);
            }
            fwrite(rgb.data(), 1, rgb.size(), f);
            fclose(f);
        }
        note("present readback frame=%llu serial=%llu format=%d nonzero=%zu",
             (unsigned long long)frames.load(), (unsigned long long)t->serial, src->format, nonzero);
    }
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &rendered;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;
    pi.pImageIndices = &index;
    check(vkQueuePresentKHR(queue, &pi), "present");
    vkQueueWaitIdle(queue);
    frames++;
    if (frames % 10 == 0)
        stats();
}
extern "C" void mtl_pool_add(void *p, size_t n) { nvn_dirty_add_range(p,n); }
extern "C" void mtl_pool_del(void *p) { nvn_dirty_remove_range(p); }
extern "C" void mtl_reg_tex(int id, const tex_t *t, const view_t *v) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (t)
        registrations[id] = {*t, v ? *v : view_t{}, v != nullptr};
    else
        registrations.erase(id);
}
extern "C" void mtl_reg_samp(int id, const samp_par *p) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    samplerParams[id] = *p;
    auto it = samplers.find(id);
    if (it != samplers.end()) {
        vkDestroySampler(dev, it->second, nullptr);
        samplers.erase(it);
    }
}
extern "C" void mtl_set_tex_pool_n(int) {}
extern "C" void mtl_tex_release(tex_t *t) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (t && t->serial)
        retiredTextures.emplace(serial.load() + 16, t->serial);
}
extern "C" void mtl_prog_release(prog_t *) {}
extern "C" void mtl_finish() {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (ready)
        vkQueueWaitIdle(queue);
}
extern "C" int mtl_wait(uint64_t s, uint64_t) { return serial.load() >= s; }
extern "C" void mtl_read_texels(const tex_t *t, unsigned level, unsigned layer, const NVNcopyRegion *r,
                                uint8_t *dst, size_t row, size_t img) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!t || !dst || !r)
        return;
    int bw, bh, bpp = nvn_fmt_bpb(t->d.format, &bw, &bh);
    if (!bpp)
        return;
    size_t pitch = ((t->d.width + bw - 1) / bw) * bpp, rows = (t->d.height + bh - 1) / bh;
    std::vector<uint8_t> linear(pitch * rows);
    if (t->mem && !nvn_deswizzle(&t->d, level, layer, t->mem, linear.data(), pitch)) {
        size_t n = ((r->width + bw - 1) / bw) * bpp;
        if (!row)
            row = n;
        if (!img)
            img = row * ((r->height + bh - 1) / bh);
        for (int y = 0; y < (r->height + bh - 1) / bh; y++)
            memcpy(dst + y * row, linear.data() + (y + r->yoffset / bh) * pitch + r->xoffset / bw * bpp, n);
    }
}
static void replayCleanup() {
    if (dev)
        vkDeviceWaitIdle(dev);
    ready = false;
    for (auto &[k, p] : pipelines) {
        if (p.pipe)
            vkDestroyPipeline(dev, p.pipe, nullptr);
        if (p.layout)
            vkDestroyPipelineLayout(dev, p.layout, nullptr);
    }
    pipelines.clear();
    for (auto &[k, s] : shaders) {
        if (s.module)
            vkDestroyShaderModule(dev, s.module, nullptr);
        if (s.layout)
            vkDestroyDescriptorSetLayout(dev, s.layout, nullptr);
    }
    shaders.clear();
    programs.clear();
    for (auto &[k, p] : passes)
        vkDestroyRenderPass(dev, p.pass, nullptr);
    passes.clear();
    for (auto &[k, t] : textures) {
        for (auto &v : t.views) vkDestroyImageView(dev, v.second, nullptr);
        vkDestroyImageView(dev, t.view, nullptr);
        vkDestroyImage(dev, t.image, nullptr);
        vkFreeMemory(dev, t.memory, nullptr);
    }
    textures.clear();
    for (auto &[k, s] : samplers)
        vkDestroySampler(dev, s, nullptr);
    samplers.clear();
    for (auto &page : arenaPages) {
        arena = page;
        vkUnmapMemory(dev, arena.mem);
        vkDestroyBuffer(dev, arena.b, nullptr);
        vkFreeMemory(dev, arena.mem, nullptr);
    }
    arenaPages.clear();
    arena = {};
    if (descpool)
        vkDestroyDescriptorPool(dev, descpool, nullptr);
    if (cmdpool)
        vkDestroyCommandPool(dev, cmdpool, nullptr);
    if (acquired)
        vkDestroySemaphore(dev, acquired, nullptr);
    if (rendered)
        vkDestroySemaphore(dev, rendered, nullptr);
    if (swapchain)
        vkDestroySwapchainKHR(dev, swapchain, nullptr);
    if (dev)
        vkDestroyDevice(dev, nullptr);
    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance)
        vkDestroyInstance(instance, nullptr);
    dev = {};
    instance = {};
    surface = {};
    swapchain = {};
    cmdpool = {};
    descpool = {};
    acquired = {};
    rendered = {};
}
extern "C" void dmReplayLoop(OHNativeWindow *window, volatile int *running) {
    {
        std::lock_guard<std::recursive_mutex> lock(mu);
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "D2R NVN Vulkan";
        app.apiVersion = VK_API_VERSION_1_1;
        const char *exts[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_OHOS_SURFACE_EXTENSION_NAME};
        VkInstanceCreateInfo ic{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ic.pApplicationInfo = &app;
        ic.enabledExtensionCount = 2;
        ic.ppEnabledExtensionNames = exts;
        if (vkCreateInstance(&ic, nullptr, &instance)) {
            note("instance failed");
            return;
        }
        VkSurfaceCreateInfoOHOS sc{VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS};
        sc.window = window;
        if (vkCreateSurfaceOHOS(instance, &sc, nullptr, &surface)) {
            note("surface failed");
            replayCleanup();
            return;
        }
        uint32_t n = 0;
        vkEnumeratePhysicalDevices(instance, &n, nullptr);
        std::vector<VkPhysicalDevice> devices(n);
        vkEnumeratePhysicalDevices(instance, &n, devices.data());
        if (!n) {
            replayCleanup();
            return;
        }
        physical = devices[0];
        vkGetPhysicalDeviceProperties(physical, &properties);
        uint32_t nq;
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &nq, nullptr);
        std::vector<VkQueueFamilyProperties> qf(nq);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &nq, qf.data());
        bool found = false;
        for (unsigned i = 0; i < nq; i++) {
            VkBool32 supported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &supported);
            if (supported && (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                qfamily = i;
                found = true;
                break;
            }
        }
        if (!found) {
            replayCleanup();
            return;
        }
        float priority = 1;
        VkDeviceQueueCreateInfo qc{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qc.queueFamilyIndex = qfamily;
        qc.queueCount = 1;
        qc.pQueuePriorities = &priority;
        VkPhysicalDeviceShaderDrawParametersFeatures draw{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES};
        VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features.pNext = &draw;
        vkGetPhysicalDeviceFeatures2(physical, &features);
        const char *de[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dc.pNext = &features;
        dc.queueCreateInfoCount = 1;
        dc.pQueueCreateInfos = &qc;
        dc.enabledExtensionCount = 1;
        dc.ppEnabledExtensionNames = de;
        VkResult result = vkCreateDevice(physical, &dc, nullptr, &dev);
        if (result) {
            note("device failed %d", result);
            replayCleanup();
            return;
        }
        vkGetDeviceQueue(dev, qfamily, 0, &queue);
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);
        extent = caps.currentExtent;
        uint32_t nf;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &nf, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(nf);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &nf, formats.data());
        VkSurfaceFormatKHR fmt = formats[0];
        for (auto f : formats)
            if (f.format == VK_FORMAT_R8G8B8A8_UNORM) {
                fmt = f;
                break;
            }
        VkSwapchainCreateInfoKHR sw{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        sw.surface = surface;
        sw.minImageCount = std::max(2u, caps.minImageCount);
        if (caps.maxImageCount)
            sw.minImageCount = std::min(sw.minImageCount, caps.maxImageCount);
        sw.imageFormat = fmt.format;
        sw.imageColorSpace = fmt.colorSpace;
        sw.imageExtent = extent;
        sw.imageArrayLayers = 1;
        sw.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        sw.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sw.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                              ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                              : caps.currentTransform;
        note("surface transform current=%u supported=%u chosen=%u", caps.currentTransform,
             caps.supportedTransforms, sw.preTransform);
        sw.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        sw.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        sw.clipped = VK_TRUE;
        result = vkCreateSwapchainKHR(dev, &sw, nullptr, &swapchain);
        if (result) {
            note("swapchain failed %d", result);
            replayCleanup();
            return;
        }
        VkSemaphoreCreateInfo sm{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(dev, &sm, nullptr, &acquired);
        vkCreateSemaphore(dev, &sm, nullptr, &rendered);
        VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pc.queueFamilyIndex = qfamily;
        pc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(dev, &pc, nullptr, &cmdpool);
        VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        ca.commandPool = cmdpool;
        ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ca.commandBufferCount = 1;
        vkAllocateCommandBuffers(dev, &ca, &cmd);
        VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 65536},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16384},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65536},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 65536},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8192},
                                        {VK_DESCRIPTOR_TYPE_SAMPLER, 16384},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 16384},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 16384}};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dp.maxSets = 8192;
        dp.poolSizeCount = 8;
        dp.pPoolSizes = sizes;
        check(vkCreateDescriptorPool(dev, &dp, nullptr, &descpool), "descriptor pool");
        arena =
            buffer(128 << 20, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                                  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
        arenaPages.push_back(arena);
        {
            auto blob = read(std::string(base) + "shaders/constants.bin");
            if (blob.size() >= 4) {
                uint32_t n;
                memcpy(&n, blob.data(), 4);
                size_t at = 4;
                for (unsigned i = 0; i < n && at + 20 <= blob.size(); i++) {
                    std::string key((char *)blob.data() + at, 16);
                    at += 16;
                    uint32_t size;
                    memcpy(&size, blob.data() + at, 4);
                    at += 4;
                    if (at + size > blob.size())
                        break;
                    shaderConstants[key] = std::vector<uint8_t>(blob.begin() + at, blob.begin() + at + size);
                    at += size;
                }
            }
        }
        ready = true;
        note("ready %s %ux%u qfamily=%u", properties.deviceName, extent.width, extent.height, qfamily);
        RLOG("NVN Vulkan replay ready %{public}s", properties.deviceName);
    }
    while (*running)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::lock_guard<std::recursive_mutex> lock(mu);
    stats();
    replayCleanup();
}
