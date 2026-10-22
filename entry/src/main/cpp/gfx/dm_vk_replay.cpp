#include "gfx/nvn/nvn_dirty.h"
#include "gfx/nvn/index_range.h"
#include "gfx/nvn/occlusion_counter.h"
#include "gfx/nvn/texture_copy.h"
// Vulkan replay of the upstream NVN record ABI. All submissions and presentation
// share one mutex; submission slots retain resources until GPU completion.
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
#include <sys/stat.h>
#include <map>
#include <mutex>
#include <native_window/external_window.h>
#include <string>
#include <string_view>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
extern "C" {
#include "gfx/tex/nvn_bcdec.h"
#include "gfx/tex/nvn_deswizzle.h"
void dmNvnShaderKey(const void *, char out[17]);
unsigned dmNvnFallbackDrawMask(void);
void dmNvnTraceFallbacks(FILE *out);
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
VkSwapchainCreateInfoKHR swapchainConfig{};
bool swapchainNeedsRebuild{};
VkExtent2D extent{};
uint32_t qfamily{};
// `cmd` mirrors the recording slot's command buffer; recording is single-threaded
// under `mu` while submissions complete asynchronously on their per-slot fences.
VkCommandBuffer cmd{};
VkDescriptorPool setpool{};      // descriptor-set cache pool; sets live across frames
VkPipelineCache pipelineCache{};
VkShaderModule waterDebugModule{};
VkPhysicalDeviceProperties properties{};
struct Buffer {
    VkBuffer b{};
    VkDeviceMemory mem{};
    uint8_t *map{};
    size_t size{};
};
// Immutable staging (zero/support slices, per-shader constant banks, content-keyed
// driver banks) lives in append-only pages that never unwind; per-submit scratch
// lives in each slot's own rewinding arena.
Buffer persistentArena;
std::vector<Buffer> persistentPages;
size_t persistentCursor{};
struct Slice {
    VkBuffer b{};
    size_t off{}, size{};
    uint8_t *mapped{};
};
std::map<std::pair<uint64_t, uint64_t>, Slice> copies;
std::map<std::pair<uint64_t, uint64_t>, Slice> dynCopies;
std::map<std::tuple<uint64_t, int, int>, uint32_t> indexRanges;
void invalidateCopies() {
    copies.clear();
    dynCopies.clear();
    indexRanges.clear();
}
// Uniform updates must not discard unrelated vertex/index/storage snapshots.
// Every cached guest slice is at most 64 MiB; start at the earliest possible
// overlapping address and keep immutable slices already referenced by draws.
void invalidateCopies(uint64_t address, uint64_t size) {
    if (!size)
        return;
    uint64_t end = address + size;
    if (end < address) {
        invalidateCopies();
        return;
    }
    uint64_t first = address > (64ull << 20) ? address - (64ull << 20) : 0;
    for (auto it = copies.lower_bound({first, 0}); it != copies.end() && it->first.first < end;) {
        if (it->first.second > address - std::min(address, it->first.first))
            it = copies.erase(it);
        else
            ++it;
    }
    for (auto it = dynCopies.lower_bound({first, 0}); it != dynCopies.end() && it->first.first < end;) {
        if (it->first.second > address - std::min(address, it->first.first))
            it = dynCopies.erase(it);
        else
            ++it;
    }
    for (auto it = indexRanges.lower_bound({first, 0, 0});
         it != indexRanges.end() && std::get<0>(it->first) < end;) {
        auto [start, count, type] = it->first;
        uint64_t bytes = uint64_t(count) * (type == 2 ? 4 : type == 1 ? 2 : 1);
        if (bytes > address - std::min(address, start))
            it = indexRanges.erase(it);
        else
            ++it;
    }
}
Slice zeroSlice{}, supportSlice{};
std::map<std::string, Slice> constantSlices;
struct Writeback {
    uint64_t addr;
    Slice slice;
    size_t size;
    bool hostReady{}; // CPU-emulated kernel result; no GPU wait needed to consume it
};
struct GpuCapture { std::string name; Slice data; };
struct DeferredEvent {   // guest-visible event write, applied on GPU completion
    uint32_t *ptr;
    uint32_t mode, value;
};
struct DeferredCounter { // guest-visible counter report, applied on GPU completion
    uint64_t *ptr;
    uint64_t a, b;
    OcclusionSnapshot samples;
};
uint64_t draws{}, dispatches{}, uploads{}, missing{}, pipelineErrors{}, submits{};
using Clock = std::chrono::steady_clock;
uint64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}
double replayMs{}, gpuMs{}, writebackMs{};
double descriptorMs{}, stagingMs{}, indexMs{}, submitLatencyMs{};
uint64_t stagedBytes{};
uint64_t setAllocs{}, setHits{}, setInvalidations{}, bankHits{}, bankBuilds{}, syncDrains{};
uint64_t submittedSerial{}, completedSerial{};   // completed = GPU done AND effects applied
std::atomic<int> armedCapture{0};
uint64_t capturedFrames{};
FILE *captureTrace{};
std::set<uint64_t> captureBuffers;
uint64_t captureSequence{};
bool geometryCapture{};
size_t geometryCaptureBytes{};
uint64_t gpuWriteVersion{};
std::map<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>, std::string> geometrySlices;
// Real occlusion queries for NVN_COUNTER_TYPE_SAMPLES_PASSED: a zero report
// tells the game "fully occluded" and it culls light sources and items (the
// upstream Metal port documents this exact failure).
enum { QUERIES_PER_SLOT = 8192 };
VkQueryPool queryPool{};
Buffer queryResults;
int queryCount{}, queryOpen{-1};
OcclusionCounter occlusion;
uint64_t queryReports{}, queryZeros{}, queryFallbacks{}, passCount{};
struct Timer {
    double &total;
    unsigned weight;
    Clock::time_point start;
    explicit Timer(double &value) : total(value) {
        // Two system clock calls per tiny binding/upload became a measurable
        // render-thread cost on device. Sample phases with probability 1/32;
        // frame rate and full submission/wait timings remain measured in full.
        // Pseudorandom selection avoids sampling the same draw phase each time.
        static thread_local uint32_t state = 0x98a54c31;
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        weight = (state & 31) == 0 ? 32 : 0;
        if (weight) start = Clock::now();
    }
    ~Timer() {
        if (weight)
            total += weight * std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }
};
Clock::time_point recordingStart;
bool deviceLost{};
// Bisect switches (compiled default 0 = all optimizations on): 8 residency
// off, 32 descriptor-cache off, 64 bank-cache off, 16 fully synchronous
// finish, 128 legacy synchronous present.
unsigned debugFlags = 0;
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
void stats();
bool check(VkResult r, const char *where) {
    if (r != VK_SUCCESS) {
        note("%s: VkResult=%d submit=%llu draws=%llu dispatch=%llu", where, r, (unsigned long long)submits,
             (unsigned long long)draws, (unsigned long long)dispatches);
        if (r == VK_ERROR_DEVICE_LOST) {
            deviceLost = true;
            note("last graphics=%s compute=%s", lastGraphics.c_str(), lastCompute.c_str());
            stats();
        }
        RLOG("replay %{public}s: %{public}d", where, r);
        return false;
    }
    return true;
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
struct ResidentBuffer {
    Buffer buffer;
    uint64_t stamp{}, used{};
};
std::map<std::pair<uint64_t, uint64_t>, ResidentBuffer> residentBuffers;
std::vector<std::pair<uint64_t, uint64_t>> gpuWrites;
size_t residentBytes{};
uint64_t residentHits{};
void deferBufferDestroy(Buffer b);          // defined after the submission ring
void invalidateSetsForBuffer(VkBuffer b);   // defined with the descriptor cache
void invalidateSetsForTexture(uint64_t serial, uint32_t gen);
void drainIfOverlap(uint64_t addr, uint64_t n);
void endpass();
void begin();
void finish(VkSemaphore wait = {}, VkSemaphore signal = {});
void reclaimAll();
void destroyBuffer(Buffer &b) {
    vkUnmapMemory(dev, b.mem);
    vkDestroyBuffer(dev, b.b, nullptr);
    vkFreeMemory(dev, b.mem, nullptr);
}
void retireResident(decltype(residentBuffers)::iterator it) {
    residentBytes -= it->second.buffer.size;
    // Cached snapshots and descriptor sets must not outlive the buffer; drop
    // them now. The VkBuffer itself dies at the recording slot's completion.
    for (auto c = copies.begin(); c != copies.end();)
        c = c->second.b == it->second.buffer.b ? copies.erase(c) : std::next(c);
    invalidateSetsForBuffer(it->second.buffer.b);
    deferBufferDestroy(it->second.buffer);
    residentBuffers.erase(it);
}
bool overlaps(uint64_t a, uint64_t n, uint64_t b, uint64_t m) {
    return a <= b ? b - a < n : a - b < m;
}
void markGpuWritesUnknown(uint64_t address, uint64_t size);
void recordGpuWrite(uint64_t address, uint64_t size) {
    markGpuWritesUnknown(address, size);
    if (!address || !size) return;
    gpuWriteVersion++;
    if (std::find(gpuWrites.begin(), gpuWrites.end(), std::make_pair(address, size)) != gpuWrites.end())
        return;
    gpuWrites.emplace_back(address, size);
    // A buffer read earlier in this submit can become a compute output later.
    // Keep its allocation alive until completion, but never reuse it next frame.
    for (auto it = residentBuffers.begin(); it != residentBuffers.end();) {
        auto current = it++;
        if (overlaps(address, size, current->first.first, current->first.second))
            retireResident(current);
    }
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
    bool separate{}, writable{true}, dynamic{};
};
// A descriptor set cached across frames. `identity` fully describes the bound
// resources; a lookup only reuses an entry when the identity matches verbatim.
struct SetEntry {
    VkDescriptorSet set{};
    uint64_t hash{};
    std::vector<uint64_t> identity;
    std::vector<VkBufferView> views;                        // texel views owned here
    std::vector<VkBuffer> refBuffers;                       // retire invalidation keys
    std::vector<std::pair<uint64_t, uint32_t>> refTextures; // serial, gen
    std::vector<std::pair<int, uint32_t>> refSamplers;      // id, gen
    uint64_t lastUsed{};
    bool dead{};
};
struct RetiredSet {
    VkDescriptorSet set{};
    std::vector<VkBufferView> views;
    uint64_t freeAt{};
};
struct Shader {
    VkShaderModule module{};
    VkDescriptorSetLayout layout{};
    int stage{};
    std::string key;
    std::vector<Binding> bindings;
    std::vector<uint32_t> inputs;
    std::vector<SetEntry> entries;
    std::unordered_map<uint64_t, std::vector<uint32_t>> byHash;
    size_t dead{};
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
    uint32_t w{}, h{}, depth{}, layers{}, levels{1}, texelBytes{};
    int target{};
    uint64_t stamp{}, seen{}, serial{};
    uint32_t gen{};   // bumped when a live serial is replaced by new contents
    std::map<std::string, VkImageView> views;
    VkImageAspectFlags aspect{VK_IMAGE_ASPECT_COLOR_BIT};
};
std::map<uint64_t, Texture> textures;
std::multimap<uint64_t, uint64_t> retiredTextures;

// ---------------- submission ring ----------------
// Up to RING submissions are in flight; each owns its command pool, fence,
// present semaphores, staging arena and the list of work that must happen only
// after the GPU signals (writebacks, events, resource destruction).
enum { RING = 4 };
struct Slot {
    VkCommandPool pool{};
    VkCommandBuffer buffer{};
    VkFence fence{};
    VkSemaphore acquireSem{}, renderSem{};
    bool live{};
    uint64_t submitSerial{}, submittedAtNs{}, completedAtNs{};
    std::vector<VkFramebuffer> framebuffers;
    std::vector<Writeback> writebacks;
    std::vector<GpuCapture> captures;
    std::vector<Buffer> deadBuffers;
    std::vector<Texture> deadTextures;
    std::vector<DeferredEvent> events;
    std::vector<DeferredCounter> counters;
    std::vector<std::shared_ptr<OcclusionSegment>> queries;
    std::vector<Buffer> arenaPages;
    size_t arenaPage{}, cursor{}, dynStart{};
    Buffer arena;
};
Slot ring[RING];
int curSlot{};
Slot *cur{};
void markGpuWritesUnknown(uint64_t address, uint64_t size) {
    if (cur && !cur->live)
        for (auto &w : cur->writebacks)
            if (overlaps(address, size, w.addr, w.size)) w.hostReady = false;
}
// One shared staging ring backs every dynamic UBO/storage binding: descriptor
// sets reference it once and draws supply per-bind offsets, so sets stay
// valid while guest data moves through the ring each submission.
Buffer dynRing;
size_t dynCursor{}, dynAlign = 256;
void deferBufferDestroy(Buffer b) { if (cur) cur->deadBuffers.push_back(std::move(b)); }

// Per-submit scratch: the recording slot's arena unwinds when that slot is
// reclaimed, so nothing cached across submissions may live here.
Slice bytes(const void *p, size_t n) {
    Timer timer{stagingMs};
    stagedBytes += n;
    const size_t payload = n;
    n = std::max(n, size_t(16));
    size_t off = (cur->cursor + 255) & ~size_t(255);
    if (off + n > cur->arena.size) {
        // Advance to the next page (or page 0 of a fresh slot); grow the page
        // list only when the target index is not backed yet.
        size_t next = cur->arenaPages.empty() ? 0 : cur->arenaPage + 1;
        if (next >= cur->arenaPages.size())
            cur->arenaPages.push_back(buffer(
                std::max(size_t(96 << 20), n + 256),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT));
        cur->arenaPage = next;
        cur->arena = cur->arenaPages[next];
        cur->cursor = off = 0;
    }
    cur->cursor = off + n;
    if (p) {
        memcpy(cur->arena.map + off, p, payload);
        if (payload < n)
            memset(cur->arena.map + off + payload, 0, n - payload);
    }
    else
        memset(cur->arena.map + off, 0, n);
    return {cur->arena.b, off, n, cur->arena.map + off};
}
// Shared staging ring for dynamic-offset UBO/SSBO bindings. It rewinds lazily
// to below the oldest still-live submission's start mark; descriptor sets
// reference the ring buffer itself, never a position.
void dynEnsure(size_t n) {
    if (!dynRing.b || !dynRing.size)
        dynRing = buffer(512u << 20, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                          VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                                          VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
    if (n + 256 <= dynRing.size)
        return;
    note("dyn ring request %zu exceeds ring", n);
    abort();
}
Slice stageDyn(const void *p, size_t n) {
    Timer timer{stagingMs};
    stagedBytes += n;
    const size_t payload = n;
    n = std::max(n, size_t(16));
    dynEnsure(n);
    size_t off = (dynCursor + dynAlign - 1) & ~(dynAlign - 1);
    if (off + n > dynRing.size) {
        size_t floorDyn = cur ? cur->dynStart : dynCursor;
        for (auto &s : ring)
            if (s.live)
                floorDyn = std::min(floorDyn, s.dynStart);
        if (n + 256 > floorDyn) {
            reclaimAll();
            floorDyn = cur ? cur->dynStart : dynCursor;
            for (auto &s : ring)
                if (s.live)
                    floorDyn = std::min(floorDyn, s.dynStart);
        }
        dynCursor = 0;
        off = 0;
        (void)floorDyn;
    }
    dynCursor = off + n;
    if (p) {
        memcpy(dynRing.map + off, p, payload);
        if (payload < n)
            memset(dynRing.map + off + payload, 0, n - payload);
    }
    else
        memset(dynRing.map + off, 0, n);
    return {dynRing.b, off, n, dynRing.map + off};
}
// Immutable staging shared across submissions; pages are never unwound.
Slice persistent(const void *p, size_t n) {
    n = std::max(n, size_t(16));
    size_t off = (persistentCursor + 255) & ~size_t(255);
    if (off + n > persistentArena.size) {
        persistentArena = buffer(std::max(size_t(8 << 20), n + 256),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
        persistentPages.push_back(persistentArena);
        persistentCursor = off = 0;
    }
    persistentCursor = off + n;
    if (p)
        memcpy(persistentArena.map + off, p, n);
    else
        memset(persistentArena.map + off, 0, n);
    return {persistentArena.b, off, n, persistentArena.map + off};
}
uint64_t fnv1a(const void *p, size_t n, uint64_t h = 1469598103934665603ull) {
    for (const uint8_t *b = (const uint8_t *)p, *e = b + n; b < e;)
        h = (h ^ *b++) * 1099511628211ull;
    return h;
}
// Content-keyed driver-bank slices: identical binding tables reuse one
// persistent snapshot, giving descriptor sets a stable identity across frames.
std::map<uint64_t, std::pair<Slice, std::vector<uint8_t>>> bankCache;
Slice bankSlice(const void *p, size_t n) {
    if (debugFlags & 64)   // bisect switch: transient arena, no cross-frame bank
        return bytes(p, n);
    // libc++ hashes machine words; byte-at-a-time FNV forms a 4096-step
    // dependency chain per bank. Full byte comparison below still proves hits.
    uint64_t h = std::hash<std::string_view>{}({static_cast<const char *>(p), n});
    auto it = bankCache.find(h);
    if (it != bankCache.end() && it->second.second.size() == n &&
        !memcmp(it->second.second.data(), p, n)) {
        bankHits++;
        return it->second.first;
    }
    Slice s = persistent(p, n);
    bankCache[h] = {s, std::vector<uint8_t>((const uint8_t *)p, (const uint8_t *)p + n)};
    bankBuilds++;
    return s;
}
Slice resident(uint64_t address, size_t size) {
    if (size < 16384 || !nvn_dirty_tracked((void *)(uintptr_t)address, size)) return {};
    for (auto [start, n] : gpuWrites)
        if (overlaps(address, size, start, n)) return {};
    auto key = std::make_pair(address, uint64_t(size));
    auto it = residentBuffers.lower_bound(key);
    if (it != residentBuffers.end() && it->first.first == address) {
        if (!nvn_dirty_since((void *)(uintptr_t)address, it->first.second, it->second.stamp)) {
            it->second.used = serial.load();
            residentHits++;
            auto &b = it->second.buffer;
            return {b.b, 0, size, b.map};
        }
        retireResident(it);
    }
    // Bound both memory use and Vulkan allocation count. Evicted buffers are
    // destroyed at the recording slot's completion.
    while (!residentBuffers.empty() &&
           (residentBytes + size > (192u << 20) || residentBuffers.size() >= 768)) {
        auto victim = std::min_element(residentBuffers.begin(), residentBuffers.end(),
            [](const auto &a, const auto &b) { return a.second.used < b.second.used; });
        retireResident(victim);
    }
    Timer timer{stagingMs};
    uint64_t stamp = nvn_dirty_now();
    nvn_dirty_protect((void *)(uintptr_t)address, size);
    Buffer b = buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
    memcpy(b.map, (void *)(uintptr_t)address, size);
    stagedBytes += size;
    residentBuffers.emplace(key, ResidentBuffer{b, stamp, serial.load()});
    residentBytes += size;
    return {b.b, 0, size, b.map};
}
Slice guest(uint64_t addr, size_t n, bool readOnly = false, bool keep = false, bool dyn = false,
            size_t alignment = 1) {
    if (!addr || !n)
        return zeroSlice.b ? zeroSlice : (zeroSlice = persistent(nullptr, 65536));
    n = std::min(n, size_t(64 << 20));
    // A later binding can select the opaque/transparent HALF of a compute
    // output, with a different guest base address. Exact-start cache lookup
    // alone silently snapshots last frame's CPU shadow instead of GPU results.
    if (cur) for (auto it = cur->writebacks.rbegin(); it != cur->writebacks.rend(); ++it) {
        if (!overlaps(addr, n, it->addr, it->size)) continue;
        if (addr >= it->addr && addr - it->addr <= it->size &&
            n <= it->size - (addr - it->addr)) {
            size_t delta = size_t(addr - it->addr);
            Slice slice = it->slice;
            if ((slice.off + delta) % alignment == 0) {
                slice.off += delta;
                slice.mapped += delta;
                slice.size = n;
                static unsigned logged = 0;
                if (delta && logged++ < 8)
                    RLOG("GPU subrange forwarded offset=%{public}zu bytes=%{public}zu parent=%{public}zu",
                         delta, n, it->size);
                return slice;
            }
            // A legal NVN subrange can require stricter host descriptor
            // alignment. Copy the GPU bytes to aligned staging, never to CPU.
            endpass();
            Slice aligned = dyn ? stageDyn(nullptr, n) : bytes(nullptr, n);
            VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            before.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            before.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 1, &before, 0, nullptr, 0, nullptr);
            VkBufferCopy region{slice.off + delta, aligned.off, n};
            vkCmdCopyBuffer(cmd, slice.b, aligned.b, 1, &region);
            VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
            after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            after.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 0, 1, &after, 0, nullptr, 0, nullptr);
            cur->writebacks.push_back({addr, aligned, n});
            return aligned;
        }
        break;
    }
    // Descriptor layout (dynamic offset vs static offset) must never split
    // guest memory identity. A compute storage write and a later vertex/fragment
    // read of the same range must reference the SAME Vulkan buffer slice.
    auto &cache = copies;
    auto key = std::make_pair(addr, (uint64_t)n);
    auto it = cache.lower_bound(key);
    if (it != cache.end() && it->first.first == addr &&
        (it->first.second == n || n >= 16)) {
        // Several meshes bind different prefixes of the same vertex/storage
        // buffer. Reuse an existing larger snapshot rather than recopy it for
        // every prefix; expose only the requested descriptor range.
        Slice s = it->second;
        s.size = std::max(n, size_t(16));
        return s;
    }
    // Render diagnostic flag 8 disables residency for an in-place A/B test.
    Slice s = keep && !(debugFlags & 8) ? resident(addr, n) : Slice{};
    if (!s.b)
        s = dyn ? stageDyn((const void *)(uintptr_t)addr, n) : bytes((const void *)(uintptr_t)addr, n);
    cache.emplace(key, s);
    return s;
}
struct Registration {
    tex_t tex{};
    view_t view{};
    bool hasView{};
};
std::map<int, Registration> registrations;
std::map<int, samp_par> samplerParams;
std::map<int, VkSampler> samplers;
std::map<int, uint32_t> samplerGen;   // bumped when a sampler id is re-registered
std::vector<RetiredSet> retiredSets;  // freed once the GPU passes lastUsed
std::vector<std::pair<VkSampler, uint64_t>> retiredSamplers;
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
    int64_t rowStride{}, imageStride{};
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
void closeQuery() {
    if (queryOpen < 0) return;
    endpass();
    vkCmdEndQuery(cmd, queryPool, uint32_t(queryOpen));
    queryOpen = -1;
}
void resumeQuery() {
    if (!occlusion.counting || queryOpen >= 0) return;
    auto segment = occlusion.segment();
    if (!queryPool || queryCount == QUERIES_PER_SLOT || (debugFlags & 256)) {
        // An unavailable measurement must not mean "fully occluded".
        segment->value = 1;
        segment->ready = true;
        queryFallbacks++;
        return;
    }
    queryOpen = int((cur - ring) * QUERIES_PER_SLOT + queryCount++);
    cur->queries.push_back(std::move(segment));
    vkCmdBeginQuery(cmd, queryPool, uint32_t(queryOpen), 0);
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
    out.h = t.d.target == 0 || t.d.target == 3 ? 1 : std::max(1u, t.d.height);
    out.depth = t.d.target == 2 ? std::max(1u, t.d.depth) : 1;
    uint64_t layerCount = textureLayerCount(t.d);
    if (layerCount > UINT32_MAX) return {};
    out.layers = uint32_t(layerCount);
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
    if (bc) {
        uint32_t unused; int pixelBytes{};
        nvn_bc_unpacked(t.d.format, &unused, &pixelBytes);
        out.texelBytes = pixelBytes;
    } else if (bw == 1 && bh == 1) out.texelBytes = bpb;
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
    value.stamp = stamp; value.seen = stamp; value.serial = t->serial;
    if (!value.image) return nullptr;
    if (it != textures.end()) {
        value.gen = it->second.gen + 1;
        invalidateSetsForTexture(t->serial, it->second.gen);
        cur->deadTextures.push_back(std::move(it->second));
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
        // Uncompressed color formats in the same texel-size class can share a
        // mutable image. Initialization uses integer views of float textures.
        int bw, bh;
        int viewBytes = nvn_fmt_bpb(q.format, &bw, &bh);
        bool uncompressed = bw == 1 && bh == 1;
        if (t.aspect == VK_IMAGE_ASPECT_COLOR_BIT && uncompressed &&
            viewBytes > 0 && unsigned(viewBytes) == t.texelBytes)
            ci.format = f;
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
    uint32_t dynU{}, dynS{};
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
                    // Ryujinx BufferUsageFlags.Write is bit 0. Missing
                    // reflection stays conservative; explicit read-only buffers
                    // must never be copied back over concurrently updated data.
                    b.writable = (field(o, "flags", 1) & 1) != 0;
                }
            for (auto &section : {"textures", "images"})
                for (auto &o : objects(json, section))
                    if (field(o, "binding", -1) == int(b.binding)) {
                        b.handle = field(o, "handle");
                        b.cbuf = field(o, "cbuf_slot", -1);
                        b.separate = field(o, "separate") != 0;
                        b.count = field(o, "array", 1);
                    }
            // Dynamic-offset bindings: the set references the shared ring and
            // each draw supplies the offset, so per-frame data churn stops
            // invalidating cached sets. Device limits cap how many may be
            // dynamic; the rest fall back to per-slice descriptors.
            // The device dynamic-offset limits apply to the whole pipeline
            // layout (vertex + fragment sets summed), so each stage gets half.
            if (b.kind == 1 && b.slot >= 3 && b.slot < 19 &&
                dynU < properties.limits.maxDescriptorSetUniformBuffersDynamic / 2) {
                b.dynamic = true;
                dynU++;
            } else if (b.kind == 2 &&
                       dynS < properties.limits.maxDescriptorSetStorageBuffersDynamic / 2) {
                b.dynamic = true;
                dynS++;
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
    for (auto &b : sh.bindings) {
        VkDescriptorType t = b.type;
        if (b.dynamic) {
            if (t == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                t = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            else if (t == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                t = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }
        layout.push_back({b.binding, t, (uint32_t)b.count,
                          sh.stage == 0   ? VK_SHADER_STAGE_VERTEX_BIT
                          : sh.stage == 1 ? VK_SHADER_STAGE_FRAGMENT_BIT
                                          : VK_SHADER_STAGE_COMPUTE_BIT,
                          nullptr});
    }
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
    cur->framebuffers.push_back(fb);
    VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rb.renderPass = currentPass;
    rb.framebuffer = fb;
    rb.renderArea = {{0, 0}, {passW, passH}};
    resumeQuery();
    vkCmdBeginRenderPass(cmd, &rb, VK_SUBPASS_CONTENTS_INLINE);
    passCount++;
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
    const bool water = !compute && vs->key == "a9a839a220417792";
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
        unsigned pipelineFlags = debugFlags & 7;
        if (water) pipelineFlags |= debugFlags & (1024 | 2048);
        key.append((char *)&pipelineFlags, sizeof pipelineFlags);
    }
    auto it = pipelines.find(key);
    if (it != pipelines.end())
        return it->second.pipe ? &it->second : nullptr;
    Pipeline out;
    VkDescriptorSetLayout layouts[2]{vs->layout, ps ? ps->layout : VK_NULL_HANDLE};
    VkPipelineLayoutCreateInfo lc{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    lc.setLayoutCount = ps ? 2 : 1;
    lc.pSetLayouts = layouts;
    if (!check(vkCreatePipelineLayout(dev, &lc, nullptr, &out.layout), "pipeline layout"))
        return &pipelines.emplace(key, out).first->second;   // cache the failure
    VkPipelineShaderStageCreateInfo stages[2]{};
    for (int i = 0; i < (ps ? 2 : 1); i++) {
        stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[i].stage = compute ? VK_SHADER_STAGE_COMPUTE_BIT
                          : i     ? VK_SHADER_STAGE_FRAGMENT_BIT
                                  : VK_SHADER_STAGE_VERTEX_BIT;
        stages[i].module = i ? ps->module : vs->module;
        stages[i].pName = "main";
    }
    if (water && ps && (debugFlags & 2048)) {
        if (!waterDebugModule) {
            auto code = read(std::string(base) + "dm_water_flat.spv");
            if (code.size() >= 20 && code.size() % 4 == 0) {
                VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
                info.codeSize = code.size();
                info.pCode = reinterpret_cast<const uint32_t *>(code.data());
                check(vkCreateShaderModule(dev, &info, nullptr, &waterDebugModule), "water diagnostic shader");
            }
        }
        if (waterDebugModule) stages[1].module = waterDebugModule;
    }
    VkResult result;
    if (compute) {
        if (submits < 2)
            note("create compute %s", vs->key.c_str());
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage = stages[0];
        ci.layout = out.layout;
        result = vkCreateComputePipelines(dev, pipelineCache, 1, &ci, nullptr, &out.pipe);
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
        if (water && (debugFlags & 2048)) rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = S.poly.front ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rs.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        std::vector<VkPipelineColorBlendAttachmentState> blend;
        for (int i = 0; i < S.rt.n; i++) {
            VkPipelineColorBlendAttachmentState c{};
            c.colorWriteMask = (S.mask.mask >> (i * 4)) & 15;
            c.blendEnable = (S.color.blend_en >> i) & 1;
            if (water && (debugFlags & 2048)) { c.blendEnable = false; c.colorWriteMask = 15; }
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
        if (water && (debugFlags & (1024 | 2048))) ds.depthTestEnable = false;
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
        result = vkCreateGraphicsPipelines(dev, pipelineCache, 1, &ci, nullptr, &out.pipe);
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

struct DriverBankLast {
    Slice slice{};
    bool valid{};
};
DriverBankLast lastDriverBank[6][2];
void updateDriverTable(void *dst, const void *src, size_t n, int stage, int bank) {
    if (!memcmp(dst, src, n)) return;
    memcpy(dst, src, n);
    lastDriverBank[stage][bank].valid = false;
}
Slice bindingBuffer(const Binding &b, Shader &sh) {
    int stage = sh.stage;
    if (b.kind == 0) {
        if (supportSlice.b)
            return supportSlice;
        std::array<uint8_t, 65536> support{};
        for (int i = 0; i < 73; i++)
            *(float *)(support.data() + 192 + 16 * i) = 1;
        return supportSlice = persistent(support.data(), support.size());
    }
    if (b.kind == 1) {
        if (b.slot >= 3 && b.slot < 19) {
            auto &u = S.ub[stage][b.slot - 3];
            return guest(u.addr, u.size, true, !b.dynamic, b.dynamic,
                         properties.limits.minUniformBufferOffsetAlignment);
        }
        if (b.slot == 1) {
            auto it = constantSlices.find(sh.key);
            if (it != constantSlices.end())
                return it->second;
        }
        // Only binding commands change these tables. Reuse the snapshot before
        // constructing/hashing 4 KiB again for every draw.
        auto &last = lastDriverBank[stage][b.slot == 2 ? 1 : 0];
        if ((b.slot == 0 || b.slot == 2) && !(debugFlags & 64) && last.valid) {
            bankHits++;
            return last.slice;
        }
        // Driver banks mirror the current binding table; identical tables reuse
        // one content-keyed persistent slice so descriptor identities stay stable.
        uint8_t bank[4096]{};
        if (b.slot == 2) {
            memcpy(bank + 0x168, S.tex[stage], sizeof S.tex[stage]);
            memcpy(bank + 0x568, S.samp[stage], sizeof S.samp[stage]);
            memcpy(bank + 0x120, S.img[stage], 8 * 8);
        }
        if (b.slot == 0) {
            int offset = stage == 0 ? 0x110 : stage == 1 ? 0x510 : 0x310;
            for (int i = 0; i < 16; i++) {
                memcpy(bank + offset + 16 * i, &S.ssbo[stage][i].addr, 8);
                uint32_t sz = S.ssbo[stage][i].size;
                memcpy(bank + offset + 16 * i + 8, &sz, 4);
            }
        }
        if (submits < 2)
            note("driver bank %d stage=%d", b.slot, stage);
        if (b.slot == 1) {
            std::array<uint8_t, 65536> driver{};
            auto it = shaderConstants.find(sh.key);
            if (it != shaderConstants.end())
                memcpy(driver.data(), it->second.data(), std::min(driver.size(), it->second.size()));
            Slice result = persistent(driver.data(), driver.size());
            constantSlices[sh.key] = result;
            return result;
        }
        Slice result = bankSlice(bank, sizeof bank);
        if ((b.slot == 0 || b.slot == 2) && !(debugFlags & 64)) {
            last.slice = result;
            last.valid = true;
        }
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
        if (b.writable) recordGpuWrite(u.addr, u.size);
        Slice s = guest(u.addr, u.size, !b.writable, !b.writable && !b.dynamic, b.dynamic,
                        properties.limits.minStorageBufferOffsetAlignment);
        if (b.writable && u.addr && u.size)
            cur->writebacks.push_back({u.addr, s, std::min(size_t(u.size), s.size)});
        return s;
    }
    return zeroSlice.b ? zeroSlice : (zeroSlice = persistent(nullptr, 65536));
}
// ---------------- descriptor-set cache ----------------
// Sets are resolved to a full identity (buffer slices, texture generations,
// view keys, sampler generations) and reused verbatim across frames. Dead
// entries are freed once the GPU has passed their last use.
void retireSet(Shader &sh, size_t idx) {
    SetEntry &e = sh.entries[idx];
    if (e.dead)
        return;
    auto bucket = sh.byHash.find(e.hash);
    if (bucket != sh.byHash.end()) {
        auto &v = bucket->second;
        for (auto it = v.begin(); it != v.end(); ++it)
            if (*it == idx) {
                *it = v.back();
                v.pop_back();
                break;
            }
    }
    e.dead = true;
    sh.dead++;
    e.identity.clear();
    e.identity.shrink_to_fit();
    e.refBuffers.clear();
    e.refBuffers.shrink_to_fit();
    e.refTextures.clear();
    e.refTextures.shrink_to_fit();
    e.refSamplers.clear();
    e.refSamplers.shrink_to_fit();
    retiredSets.push_back({e.set, std::move(e.views), e.lastUsed});
    e.set = VK_NULL_HANDLE;
    e.views.clear();
    setInvalidations++;
    // Tombstones must never make lookups or invalidation scans slower over
    // time; compact once they dominate the table.
    if (sh.dead > 32 && sh.dead * 2 > sh.entries.size()) {
        std::vector<SetEntry> live;
        live.reserve(sh.entries.size() - sh.dead);
        for (auto &entry : sh.entries)
            if (!entry.dead)
                live.push_back(std::move(entry));
        sh.entries = std::move(live);
        sh.dead = 0;
        sh.byHash.clear();
        for (size_t i = 0; i < sh.entries.size(); i++)
            sh.byHash[sh.entries[i].hash].push_back(uint32_t(i));
    }
}
void processRetiredSets() {
    for (auto it = retiredSets.begin(); it != retiredSets.end();) {
        if (it->freeAt > completedSerial) {
            ++it;
            continue;
        }
        if (it->set)
            vkFreeDescriptorSets(dev, setpool, 1, &it->set);
        for (auto v : it->views)
            vkDestroyBufferView(dev, v, nullptr);
        it = retiredSets.erase(it);
    }
}
void invalidateSetsForBuffer(VkBuffer b) {
    for (auto &[k, sh] : shaders)
        for (size_t i = 0; i < sh.entries.size(); i++)
            if (!sh.entries[i].dead &&
                std::find(sh.entries[i].refBuffers.begin(), sh.entries[i].refBuffers.end(), b) !=
                    sh.entries[i].refBuffers.end())
                retireSet(sh, i);
}
void invalidateSetsForTexture(uint64_t serial, uint32_t gen) {
    for (auto &[k, sh] : shaders)
        for (size_t i = 0; i < sh.entries.size(); i++)
            if (!sh.entries[i].dead &&
                std::find(sh.entries[i].refTextures.begin(), sh.entries[i].refTextures.end(),
                          std::make_pair(serial, gen)) != sh.entries[i].refTextures.end())
                retireSet(sh, i);
}
void invalidateAllSets() {
    for (auto &[k, sh] : shaders) {
        for (size_t i = 0; i < sh.entries.size(); i++)
            retireSet(sh, i);
        sh.byHash.clear();
    }
}
// A slot's arena memory unwinds at its completion; every entry that staged
// scratch inside it must be rebuilt afterwards.
void invalidateSetsForArena(const Slot &s) {
    for (auto &[k, sh] : shaders)
        for (size_t i = 0; i < sh.entries.size(); i++) {
            bool hit = false;
            for (VkBuffer b : sh.entries[i].refBuffers)
                for (auto &page : s.arenaPages)
                    if (b == page.b)
                        hit = true;
            if (hit && !sh.entries[i].dead)
                retireSet(sh, i);
        }
}
VkDescriptorSet descriptors(Shader &sh, std::vector<std::pair<uint32_t, uint32_t>> *dynOffsets) {
    Timer timer{descriptorMs};
    struct Resolved {
        Slice slice{};
        Texture *tex{};
        const view_t *view{};
        uint64_t viewKey{}, serial{};
        uint32_t gen{}, sampGen{};
        int sampId{-1};
        size_t range{};
        VkFormat fmt{};
    };
    // Scratch buffers are reused across calls; this path runs thousands of
    // times per frame.
    static thread_local std::vector<Resolved> rs;
    static thread_local std::vector<uint64_t> identity;
    rs.clear();
    rs.resize(sh.bindings.size());
    identity.clear();
    identity.reserve(sh.bindings.size() * 4);
    for (size_t i = 0; i < sh.bindings.size(); i++) {
        const Binding &b = sh.bindings[i];
        Resolved &r = rs[i];
        if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || b.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            r.slice = bindingBuffer(b, sh);
            if (!r.slice.b) {
                if (setAllocs + setHits < 16)
                    note("descriptors %s bail: empty slice binding=%u kind=%d slot=%d dyn=%d", sh.key.c_str(),
                         b.binding, b.kind, b.slot, int(b.dynamic));
                return {};
            }
            size_t range = std::min(r.slice.size,
                                    b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                        ? size_t(properties.limits.maxUniformBufferRange)
                                        : size_t(properties.limits.maxStorageBufferRange));
            r.range = range;
            if (captureTrace && armedCapture.load() > 0 && b.kind == 2 && sh.stage == 1) {
                fprintf(captureTrace, "F %llu %s binding=%u offset=%x buffer=%llx off=%zu size=%zu\n",
                        (unsigned long long)capturedFrames, sh.key.c_str(), b.binding, b.offset,
                        (unsigned long long)r.slice.b, r.slice.off, r.slice.size);
            }
            if (b.dynamic) {
                // Position rides in the draw's dynamic offset; the cached set
                // identity needs the backing buffer (ring vs zero slice) and
                // the declared range. Buffer-less identities conflated empty
                // bindings with real data and produced wrong-content draws.
                if (dynOffsets)
                    dynOffsets->push_back({b.binding, uint32_t(r.slice.off)});
                identity.push_back(4);
                identity.push_back((uint64_t)r.slice.b);
                identity.push_back(range);
            } else {
                identity.push_back(1);
                identity.push_back((uint64_t)r.slice.b);
                identity.push_back(r.slice.off);
                identity.push_back(range);
            }
        } else if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
                   b.type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            uint64_t h = bindingHandle(b, sh.stage);
            auto it = registrations.find(h & 0xfffff);
            if (it != registrations.end() && it->second.tex.mem) {
                auto &t = it->second.tex;
                r.slice = guest((uint64_t)(uintptr_t)t.mem, t.L.size, true, false, false,
                                properties.limits.minTexelBufferOffsetAlignment);
                r.fmt = format(t.d.format);
            } else {
                r.slice = bytes(nullptr, 4096);
                r.fmt = VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            if (captureTrace && armedCapture.load() > 0 && sh.stage == 1)
                fprintf(captureTrace, "FT %llu %s binding=%u handle=%llx buffer=%llx off=%zu size=%zu fmt=%d\n",
                        (unsigned long long)capturedFrames, sh.key.c_str(), b.binding,
                        (unsigned long long)h, (unsigned long long)r.slice.b, r.slice.off, r.slice.size, int(r.fmt));
            identity.push_back(2);
            identity.push_back((uint64_t)r.slice.b);
            identity.push_back(r.slice.off);
            identity.push_back(r.slice.size);
            identity.push_back(r.fmt);
        } else {
            uint64_t h = bindingHandle(b, sh.stage);
            r.sampId = int(h >> 20);
            r.sampGen = samplerGen.count(r.sampId) ? samplerGen[r.sampId] : 0;
            r.tex = b.kind == 4 ? nullptr : texid(h & 0xfffff);
            if (!r.tex && b.kind != 4)
                r.tex = dummyTexture(b.dim);
            if (!r.tex && b.kind != 4)
                return {};
            if (r.tex) {
                r.serial = r.tex->serial;
                r.gen = r.tex->gen;
                auto reg = registrations.find(h & 0xfffff);
                if (reg != registrations.end() && reg->second.hasView) r.view = &reg->second.view;
                r.viewKey = r.view ? fnv1a(r.view, sizeof(view_t)) : 0;
            }
            if (captureTrace && armedCapture.load() > 0 &&
                (sh.key == "a9a839a220417792" || sh.key == "3be9871c4d40cf22"))
                fprintf(captureTrace, "WATER_TEXTURE %s binding=%u handle=%llx serial=%llu gen=%u size=%ux%u depth=%u levels=%u format=%d sampler=%d\n",
                        sh.key.c_str(), b.binding, (unsigned long long)h, (unsigned long long)r.serial,
                        r.gen, r.tex ? r.tex->w : 0, r.tex ? r.tex->h : 0,
                        r.tex ? r.tex->depth : 0, r.tex ? r.tex->levels : 0,
                        r.tex ? int(r.tex->format) : 0, r.sampId);
            identity.push_back(3);
            identity.push_back(r.serial);
            identity.push_back(r.gen);
            identity.push_back(r.viewKey);
            identity.push_back((uint64_t)r.sampId);
            identity.push_back(r.sampGen);
        }
        if (submits < 2)
            note("descriptor %s binding=%u kind=%d type=%d slot=%d", sh.key.c_str(), b.binding, b.kind, b.type,
                 b.slot);
    }
    // Hash whole machine words, then prove a hit with full identity equality.
    // Byte-serial FNV here ran for every vertex/fragment set on every draw.
    uint64_t hash = std::hash<std::string_view>{}(
        {reinterpret_cast<const char *>(identity.data()), identity.size() * sizeof(uint64_t)});
    if (!(debugFlags & 32)) {   // bisect switch 32: rebuild sets every call
        auto &bucket = sh.byHash[hash];
        for (uint32_t idx : bucket) {
            SetEntry &e = sh.entries[idx];
            if (!e.dead && e.identity == identity) {
                e.lastUsed = submittedSerial + 1;
                setHits++;
                return e.set;
            }
        }
    }
    if (submits < 2)
        note("allocate descriptors %s layout=%p pool=%p", sh.key.c_str(), (void *)sh.layout, (void *)setpool);
    VkDescriptorSet set{};
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = setpool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &sh.layout;
    if (vkAllocateDescriptorSets(dev, &ai, &set)) {
        // Pool exhausted: flush everything reclaimable and retry once.
        note("descriptor pool exhausted entries=%zu retired=%zu", sh.entries.size(), retiredSets.size());
        processRetiredSets();
        if (vkAllocateDescriptorSets(dev, &ai, &set)) {
            invalidateAllSets();
            processRetiredSets();
            if (vkAllocateDescriptorSets(dev, &ai, &set)) {
                note("descriptor allocation failed for %s", sh.key.c_str());
                return {};
            }
        }
    }
    SetEntry entry;
    entry.set = set;
    entry.identity = identity;
    entry.lastUsed = submittedSerial + 1;
    static thread_local std::vector<VkWriteDescriptorSet> writes;
    static thread_local std::vector<std::vector<VkDescriptorBufferInfo>> buffers;
    static thread_local std::vector<std::vector<VkDescriptorImageInfo>> images;
    static thread_local std::vector<std::vector<VkBufferView>> texels;
    writes.clear();
    // Keep each binding's backing storage too: clearing the outer vectors
    // discarded all inner capacities and allocated them again on every miss.
    if (buffers.size() < sh.bindings.size()) buffers.resize(sh.bindings.size());
    if (images.size() < sh.bindings.size()) images.resize(sh.bindings.size());
    if (texels.size() < sh.bindings.size()) texels.resize(sh.bindings.size());
    size_t bindingIndex = 0;
    for (auto &b : sh.bindings) {
        Resolved &r = rs[bindingIndex];
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = set;
        w.dstBinding = b.binding;
        w.descriptorCount = b.count;
        w.descriptorType = b.type;
        auto &bs = buffers[bindingIndex];
        auto &is = images[bindingIndex];
        auto &views = texels[bindingIndex++];
        if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || b.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            bs.resize(b.count);
            if (b.dynamic) {
                w.descriptorType = b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                       ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                       : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                for (auto &v : bs)
                    v = {r.slice.b, 0, r.range};
            } else {
                entry.refBuffers.push_back(r.slice.b);
                for (auto &v : bs)
                    v = {r.slice.b, r.slice.off, r.range};
            }
            w.pBufferInfo = bs.data();
        } else if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
                   b.type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            views.resize(b.count);
            entry.refBuffers.push_back(r.slice.b);
            VkBufferViewCreateInfo vi{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
            vi.buffer = r.slice.b;
            vi.format = r.fmt;
            vi.offset = r.slice.off;
            vi.range = r.slice.size;
            VkBufferView view{};
            if (vkCreateBufferView(dev, &vi, nullptr, &view))
                return {};
            entry.views.push_back(view);
            std::fill(views.begin(), views.end(), view);
            w.pTexelBufferView = views.data();
        } else {
            is.resize(b.count);
            if (r.tex)
                entry.refTextures.push_back({r.serial, r.gen});
            if (r.sampId >= 0)
                entry.refSamplers.push_back({r.sampId, r.sampGen});
            for (auto &v : is)
                v = {r.sampId >= 0 ? sampler(r.sampId) : VK_NULL_HANDLE,
                     r.tex ? textureView(*r.tex, r.view) : VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL};
            w.pImageInfo = is.data();
        }
        writes.push_back(w);
    }
    vkUpdateDescriptorSets(dev, writes.size(), writes.data(), 0, nullptr);
    setAllocs++;
    if (debugFlags & 32) {   // uncached mode: retire as soon as this submit completes
        retiredSets.push_back({set, std::move(entry.views), entry.lastUsed});
        return set;
    }
    entry.hash = hash;
    sh.entries.push_back(std::move(entry));
    sh.byHash[hash].push_back(uint32_t(sh.entries.size() - 1));
    sh.logged = true;
    return set;
}
void traceDraw(const Prog &p, int count, const p_draw *dw, const p_draw_idx *di) {
    if (!captureTrace || armedCapture.load() <= 0) return;
    fprintf(captureTrace, "D %llu %llu %s %s %d %ux%u ds=%d/%d/%d blend=%u mask=%x",
            (unsigned long long)capturedFrames, (unsigned long long)draws,
            p.sh[0]->key.c_str(), p.sh[1] ? p.sh[1]->key.c_str() : "-", count,
            passW, passH, S.ds.dt, S.ds.dw, S.ds.df, S.color.blend_en, S.mask.mask);
    for (int i = 0; i < S.rt.n; ++i)
        fprintf(captureTrace, " rt%d=%llu", i, (unsigned long long)(S.rt.color[i] ? S.rt.color[i]->serial : 0));
    fprintf(captureTrace, " prim=%d first=%d base_vertex=%d base_instance=%d instances=%d stencil=%d\n",
            di ? di->prim : dw->prim, dw ? dw->first : 0, di ? di->base_vertex : 0,
            di ? di->base_instance : 0, di ? di->instances : 1, int(S.ds.st));
    for (int stage = 0; stage < 6; ++stage) {
        if (!p.sh[stage]) continue;
        bool used[16]{};
        for (const auto &b : p.sh[stage]->bindings)
            if (b.kind == 1 && b.slot >= 3 && b.slot < 19) used[b.slot - 3] = true;
        for (int i = 0; i < 16; ++i) {
            if (!used[i]) continue;
            auto u = S.ub[stage][i];
            if (!u.addr || !u.size) continue;
            size_t n = std::min(size_t(u.size), size_t(65536));
            uint64_t hash = fnv1a((const void *)(uintptr_t)u.addr, n);
            fprintf(captureTrace, "U %d %d %llx %zu %016llx\n", stage, i,
                    (unsigned long long)u.addr, n, (unsigned long long)hash);
            if (captureBuffers.insert(hash).second) {
                char name[96]; snprintf(name, sizeof name, "flash/ub_%016llx.bin", (unsigned long long)hash);
                if (FILE *f = fopen((std::string(base) + name).c_str(), "wb")) {
                    fwrite((const void *)(uintptr_t)u.addr, 1, n, f); fclose(f);
                }
            }
        }
        fprintf(captureTrace, "T %d", stage);
        for (int i = 0; i < 128; ++i)
            if (S.tex[stage][i]) fprintf(captureTrace, " %d:%llx", i, (unsigned long long)S.tex[stage][i]);
        fprintf(captureTrace, "\n");
    }
}
void captureGeometrySlice(const Slice &src, int stream, uint64_t address, int stride, int divisor) {
    if (!src.b || !src.size) return;
    // Immutable slices are shared by many draws. A GPU writer advances the
    // version, so a later changed result is never mistaken for the earlier one.
    auto key = std::make_tuple(capturedFrames, uint64_t(src.b), uint64_t(src.off),
                               uint64_t(src.size), gpuWriteVersion);
    auto found = geometrySlices.find(key);
    std::string name;
    size_t n = std::min(src.size, size_t(8 << 20));
    if (found == geometrySlices.end()) {
        if (geometryCaptureBytes + n > (128u << 20)) {
            fprintf(captureTrace, "GEOM_LIMIT %llu requested=%zu\n", (unsigned long long)draws, src.size);
            return;
        }
        endpass(); // copies are forbidden inside a render pass
        Slice dst = bytes(nullptr, n);
        VkBufferCopy copy{src.off, dst.off, n};
        vkCmdCopyBuffer(cmd, src.b, dst.b, 1, &copy);
        VkMemoryBarrier ordered{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        ordered.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        ordered.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0, 1, &ordered, 0, nullptr, 0, nullptr);
        char path[128];
        snprintf(path, sizeof path, "flash/geom_%llu_%llu.bin",
                 (unsigned long long)capturedFrames, (unsigned long long)captureSequence++);
        name = path;
        cur->captures.push_back({name, dst});
        geometrySlices.emplace(key, name);
        geometryCaptureBytes += n;
    } else name = found->second;
    fprintf(captureTrace, "V %llu stream=%d addr=%llx stride=%d divisor=%d bytes=%zu full=%zu %s\n",
            (unsigned long long)draws, stream, (unsigned long long)address, stride, divisor,
            n, src.size, name.c_str());
}
// Vulkan retains these states within a command buffer. All graphics pipelines
// declare the same six dynamic states, so unchanged values need no new command.
struct GraphicsCommands {
    VkPipeline pipeline{};
    VkViewport viewport{};
    VkRect2D scissor{};
    float blend[4]{};
    int stencilRef{}, stencilValueMask{}, stencilMask{};
    bool valid{};
} graphicsCommands;
uint64_t graphicsStateCommands{}, graphicsStateSkipped{};
void bindGraphicsPipeline(VkPipeline pipeline) {
    if (graphicsCommands.pipeline == pipeline) {
        graphicsStateSkipped++;
        return;
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    graphicsCommands.pipeline = pipeline;
    graphicsStateCommands++;
}
void dynamic() {
    auto &old = graphicsCommands;
    auto changed = [&](const void *a, const void *b, size_t n) {
        if (!old.valid || memcmp(a, b, n)) { graphicsStateCommands++; return true; }
        graphicsStateSkipped++;
        return false;
    };
    float w = S.viewport[2] > 0 ? S.viewport[2] : passW, h = S.viewport[3] > 0 ? S.viewport[3] : passH;
    VkViewport vp{S.viewport[0], S.viewport[1] + h, w, -h, S.nearZ, S.farZ};
    if (changed(&vp, &old.viewport, sizeof vp)) {
        vkCmdSetViewport(cmd, 0, 1, &vp);
        old.viewport = vp;
    }
    int x = std::max(0, S.sc.x), y = std::max(0, S.sc.y);
    uint32_t sw = S.sc.w > 0 ? std::min(uint32_t(S.sc.w), passW - std::min(passW, uint32_t(x))) : passW,
             sh = S.sc.h > 0 ? std::min(uint32_t(S.sc.h), passH - std::min(passH, uint32_t(y))) : passH;
    VkRect2D sc{{x, y}, {sw, sh}};
    if (changed(&sc, &old.scissor, sizeof sc)) {
        vkCmdSetScissor(cmd, 0, 1, &sc);
        old.scissor = sc;
    }
    if (changed(S.blendColor, old.blend, sizeof old.blend)) {
        vkCmdSetBlendConstants(cmd, S.blendColor);
        memcpy(old.blend, S.blendColor, sizeof old.blend);
    }
    if (changed(&S.stencilRef, &old.stencilRef, sizeof S.stencilRef)) {
        vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilRef);
        old.stencilRef = S.stencilRef;
    }
    if (changed(&S.stencilValueMask, &old.stencilValueMask, sizeof S.stencilValueMask)) {
        vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilValueMask);
        old.stencilValueMask = S.stencilValueMask;
    }
    if (changed(&S.stencilMask, &old.stencilMask, sizeof S.stencilMask)) {
        vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, S.stencilMask);
        old.stencilMask = S.stencilMask;
    }
    old.valid = true;
}
void draw(const p_draw *dw, const p_draw_idx *di) {
    Prog *p = program();
    if (captureTrace && armedCapture.load() > 0)
        fprintf(captureTrace, "ATTEMPT %llu %s %s count=%d\n",
                (unsigned long long)draws, p && p->sh[0] ? p->sh[0]->key.c_str() : "-",
                p && p->sh[1] ? p->sh[1]->key.c_str() : "-", di ? di->count : dw->count);
    if (!p || !p->sh[0])
        return;
    const bool water = p->sh[0]->key == "a9a839a220417792";
    lastGraphics = p->sh[0]->key + "/" + (p->sh[1] ? p->sh[1]->key : "depth");
    // Resource creation and transfer commands must precede the render pass.
    static thread_local std::vector<std::pair<uint32_t, uint32_t>> dyn0, dyn1;
    dyn0.clear();
    dyn1.clear();
    VkDescriptorSet sets[2]{descriptors(*p->sh[0], &dyn0),
                            p->sh[1] ? descriptors(*p->sh[1], &dyn1) : VK_NULL_HANDLE};
    if (!sets[0] || (p->sh[1] && !sets[1]) || !beginpass()) {
        if (water) {
            static unsigned logged = 0;
            if (logged++ < 8)
                RLOG("WATER skipped descriptor/pass vs=%{public}d ps=%{public}d pass=%{public}d",
                     sets[0] != 0, sets[1] != 0, int(inPass));
        }
        if (draws + dispatches < 16)
            note("draw bail: vs_set=%d ps_set=%d pass=%d vs=%s", sets[0] != 0, p->sh[1] ? sets[1] != 0 : 1,
                 inPass, p->sh[0]->key.c_str());
        return;
    }
    Pipeline *pipe = pipeline(*p, di ? di->prim : dw->prim);
    if (!pipe || !pipe->pipe) {
        if (water) RLOG("WATER skipped pipeline");
        if (draws + dispatches < 16)
            note("draw bail: pipe=%d vs=%s ps=%s", pipe && pipe->pipe, p->sh[0]->key.c_str(),
                 p->sh[1] ? p->sh[1]->key.c_str() : "-");
        return;
    }
    bindGraphicsPipeline(pipe->pipe);
    std::sort(dyn0.begin(), dyn0.end());
    std::sort(dyn1.begin(), dyn1.end());
    static thread_local std::vector<uint32_t> offsets;
    offsets.clear();
    offsets.reserve(dyn0.size() + dyn1.size());
    for (auto &[binding, off] : dyn0) offsets.push_back(off);
    for (auto &[binding, off] : dyn1) offsets.push_back(off);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, p->sh[1] ? 2 : 1, sets,
                            uint32_t(offsets.size()), offsets.data());
    dynamic();
    traceDraw(*p, di ? di->count : dw->count, dw, di);
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
        Timer timer{indexMs};
        size_t unit = di->itype == 2 ? 4 : di->itype == 1 ? 2 : 1;
        const uint8_t *ptr = (const uint8_t *)(uintptr_t)di->ibuf;
        if (!ptr || di->count <= 0 || di->count > 10000000)
            return;
        auto key = std::make_tuple(di->ibuf, di->count, di->itype);
        auto range = indexRanges.find(key);
        if (range != indexRanges.end()) {
            maxIndex = range->second;
        } else {
            maxIndex = nvnIndexLimit(ptr, di->count, di->itype);
            indexRanges.emplace(key, maxIndex);
        }
        if (di->itype == 0) {
            std::vector<uint16_t> expanded(di->count);
            for (int i = 0; i < di->count; i++)
                expanded[i] = ptr[i];
            indices = bytes(expanded.data(), expanded.size() * 2);
        } else
            indices = guest(di->ibuf, di->count * unit, true, true);
        it = di->itype == 2 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    }
    std::array<bool, 16> bound{};
    std::array<Slice, 16> geometryVertices;
    const bool captureMesh = geometryCapture && armedCapture.load() > 0 && S.rt.depth &&
                             passW <= 1920 && passH <= 1080;
    for (uint32_t loc : p->sh[0]->inputs) {
        if (loc >= 16)
            continue;
        int stream = S.va[loc].stream;
        if (stream < 0 || stream >= 16) continue;
        if (bound[stream])
            continue;
        bound[stream] = true;
        auto v = S.vb[stream];
        uint64_t needed =
            (uint64_t)(maxIndex + std::max(di ? di->base_vertex : 0, 0)) * S.streams[stream].stride + 256;
        if (S.streams[stream].divisor)
            needed = (uint64_t)(di ? di->instances : 1) * S.streams[stream].stride + 256;
        size_t n = std::min<uint64_t>(v.size, needed);
        Slice b = v.address && n ? guest(v.address, n, true, true) : bytes(nullptr, 4096);
        if (captureMesh) geometryVertices[stream] = b;
        VkDeviceSize off = b.off;
        vkCmdBindVertexBuffers(cmd, stream, 1, &b.b, &off);
    }
    if (di) {
        vkCmdBindIndexBuffer(cmd, indices.b, indices.off, it);
        vkCmdDrawIndexed(cmd, di->count, std::max(1, di->instances), 0, di->base_vertex, di->base_instance);
    } else if (dw->count > 0)
        vkCmdDraw(cmd, dw->count, 1, dw->first, 0);
    if (water) {
        static unsigned logged = 0;
        if (logged++ < 4)
            RLOG("WATER submitted vertices=%{public}d target=%{public}ux%{public}u depth=%{public}d/%{public}d/%{public}d",
                 di ? di->count : dw->count, passW, passH, int(S.ds.dt), int(S.ds.dw), int(S.ds.df));
    }
    if (captureMesh) {
        if (di) {
            fprintf(captureTrace, "I %llu type=%d count=%d\n", (unsigned long long)draws, di->itype, di->count);
            captureGeometrySlice(indices, -1, di->ibuf, 0, 0);
        }
        for (uint32_t loc : p->sh[0]->inputs)
            if (loc < 16)
                fprintf(captureTrace, "A %llu loc=%u stream=%u fmt=%u offset=%u\n",
                        (unsigned long long)draws, loc, S.va[loc].stream, S.va[loc].fmt, S.va[loc].offset);
        for (int stream = 0; stream < 16; ++stream)
            if (bound[stream]) captureGeometrySlice(geometryVertices[stream], stream, S.vb[stream].address,
                                                    S.streams[stream].stride, S.streams[stream].divisor);
    }
    draws++;
}
bool counterWord(const Slot &slot, uint64_t address, uint64_t &value) {
    for (auto it = slot.counters.rbegin(); it != slot.counters.rend(); ++it) {
        uint64_t start = (uint64_t)(uintptr_t)it->ptr;
        if (address == start + 8) { value = it->b; return true; }
        if (address != start) continue;
        for (const auto &part : it->samples.pending)
            if (!part->ready) return false;
        value = it->a + it->samples.value();
        return true;
    }
    return false;
}
uint64_t cpuKernelWord(uint64_t address) {
    uint64_t value{};
    if (cur && !cur->live) {
        for (auto it = cur->writebacks.rbegin(); it != cur->writebacks.rend(); ++it) {
            if (!overlaps(address, 8, it->addr, it->size)) continue;
            if (it->hostReady && address >= it->addr && address - it->addr <= it->size &&
                8 <= it->size - (address - it->addr)) {
                memcpy(&value, it->slice.mapped + address - it->addr, 8);
                return value;
            }
            // A real compute producer must complete before CPU consumption.
            drainIfOverlap(address, 8);
            memcpy(&value, (const void *)(uintptr_t)address, 8);
            return value;
        }
        for (const auto &c : cur->counters) {
            auto start = (uint64_t)(uintptr_t)c.ptr;
            if (address != start && address != start + 8) continue;
            if (counterWord(*cur, address, value)) return value;
            drainIfOverlap(address, 8);
            memcpy(&value, (const void *)(uintptr_t)address, 8);
            return value;
        }
    }
    const Slot *latest = nullptr;
    for (const auto &slot : ring)
        if (slot.live) for (const auto &c : slot.counters) {
            auto start = (uint64_t)(uintptr_t)c.ptr;
            if ((address == start || address == start + 8) &&
                (!latest || slot.submitSerial > latest->submitSerial)) latest = &slot;
        }
    if (latest && counterWord(*latest, address, value)) return value;
    drainIfOverlap(address, 8);
    memcpy(&value, (const void *)(uintptr_t)address, 8);
    return value;
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
    // Timestamp reports are known while recording. Forward their values to
    // this emulated GPU command without stalling the entire submission; publish
    // its output only with the submission's other completed writebacks.
    drainIfOverlap(u.addr, 4);
    uint32_t n;
    memcpy(&n, (void *)(uintptr_t)u.addr, 4);
    n = std::min<uint64_t>(n, std::min<uint64_t>(dst.size, 64u << 20) / 8);
    std::vector<uint64_t> result;
    result.reserve(n);
    for (unsigned i = 0; i < n; i++) {
        size_t off = i * (mode < 2 ? 16 : 32) + (mode % 2 ? 8 : 0);
        if (off + 8 > src.size) break;
        uint64_t a = cpuKernelWord(src.addr + off);
        uint64_t b = mode >= 2 && off + 24 <= src.size ? cpuKernelWord(src.addr + off + 16) : 0;
        result.push_back(mode >= 2 ? b - a : a);
    }
    const size_t bytesWritten = result.size() * 8;
    if (bytesWritten) {
        recordGpuWrite(dst.addr, bytesWritten);
        invalidateCopies(dst.addr, bytesWritten);
        Slice output = stageDyn(result.data(), bytesWritten);
        copies[{dst.addr, bytesWritten}] = output;
        cur->writebacks.push_back({dst.addr, output, bytesWritten, true});
    }
    return true;
}
void captureCompute(Shader &sh) {
    if (!captureTrace || armedCapture.load() <= 0) return;
    if (sh.key != "b54d192f7c620ded" && sh.key != "6a7a308d61928982" && sh.key != "1188eaedc8db3285") return;
    fprintf(captureTrace, "C %llu %s\n", (unsigned long long)capturedFrames, sh.key.c_str());
    for (const auto &b : sh.bindings) {
        Slice src;
        if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            uint64_t handle = bindingHandle(b, sh.stage);
            auto reg = registrations.find(handle & 0xfffff);
            if (reg == registrations.end() || !reg->second.tex.mem) continue;
            const auto &t = reg->second.tex;
            src = guest((uint64_t)(uintptr_t)t.mem, t.L.size, true);
            fprintf(captureTrace, "TBUF %llu %s binding=%u handle=%llx addr=%p bytes=%llu fmt=%d\n",
                    (unsigned long long)capturedFrames, sh.key.c_str(), b.binding,
                    (unsigned long long)handle, t.mem, (unsigned long long)t.L.size, t.d.format);
        } else {
            if (b.kind != 2 && !(b.kind == 1 && b.slot >= 3 && b.slot < 19)) continue;
            src = bindingBuffer(b, sh);
        }
        size_t n = std::min(src.size, size_t(4 << 20));
        if (!src.b || !n) continue;
        Slice dst = bytes(nullptr, n);
        VkBufferCopy copy{src.off, dst.off, n};
        vkCmdCopyBuffer(cmd, src.b, dst.b, 1, &copy);
        char name[160];
        snprintf(name, sizeof name, "flash/gpu_%llu_%s_b%u_%llu.bin",
                 (unsigned long long)capturedFrames, sh.key.c_str(), b.binding,
                 (unsigned long long)captureSequence++);
        fprintf(captureTrace, "G %u kind=%d slot=%d offset=%x writable=%d buffer=%llx off=%zu size=%zu %s\n",
                b.binding, b.kind, b.slot, b.offset, int(b.writable),
                (unsigned long long)src.b, src.off, n, name);
        cur->captures.push_back({name, dst});
    }
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);
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
    std::vector<std::pair<uint32_t, uint32_t>> dyn0;
    VkDescriptorSet set = descriptors(sh, &dyn0);
    Pipeline *pipe = pipeline(*p, 0, true);
    if (!set || !pipe || !pipe->pipe)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->pipe);
    std::sort(dyn0.begin(), dyn0.end());
    std::vector<uint32_t> offsets;
    offsets.reserve(dyn0.size());
    for (auto &[binding, off] : dyn0) offsets.push_back(off);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->layout, 0, 1, &set,
                            uint32_t(offsets.size()), offsets.data());
    vkCmdDispatch(cmd, d.x, d.y, d.z);
    dispatches++;
    VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1,
                         &mb, 0, nullptr, 0, nullptr);
    captureCompute(sh);
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
void clearTexture(const p_clear_tex &c) {
    endpass();
    Texture *t = texture(c.tex);
    if (!t || !(c.mask & 15)) return;
    if ((c.mask & 15) != 15 || t->target == 2) {
        note("unsupported texture clear mask=%x target=%d", c.mask, t->target);
        return;
    }
    view_t view = c.has_view ? c.view : view_t{};
    unsigned level = view.set_levels ? unsigned(std::max(0, view.base_level)) : 0;
    unsigned layer = (view.set_layers ? unsigned(std::max(0, view.min_layer)) : 0) +
                     unsigned(std::max(0, c.r.zoffset));
    unsigned count = unsigned(std::max(1, c.r.depth));
    if (level >= t->levels || layer >= t->layers || count > t->layers - layer ||
        c.r.xoffset < 0 || c.r.yoffset < 0 || c.r.width <= 0 || c.r.height <= 0) return;
    unsigned width = std::max(1u, t->w >> level), height = std::max(1u, t->h >> level);
    if (unsigned(c.r.xoffset) >= width || unsigned(c.r.yoffset) >= height) return;
    view.set_levels = view.set_layers = 1;
    view.base_level = level; view.nlevels = view.nlayers = 1;
    VkFormat clearFormat = view.set_format ? format(view.format) : t->format;
    std::string key = "texture-clear:" + std::to_string(clearFormat) + ":" + std::to_string(t->aspect);
    auto found = passes.find(key);
    if (found == passes.end()) {
        VkAttachmentDescription attachment{};
        attachment.format = clearFormat;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout = attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_GENERAL};
        VkSubpassDescription sub{};
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        if (t->aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
            sub.colorAttachmentCount = 1; sub.pColorAttachments = &ref;
        } else sub.pDepthStencilAttachment = &ref;
        VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        info.attachmentCount = 1; info.pAttachments = &attachment;
        info.subpassCount = 1; info.pSubpasses = &sub;
        Pass pass{};
        if (!check(vkCreateRenderPass(dev, &info, nullptr, &pass.pass), "texture clear render pass")) return;
        found = passes.emplace(key, pass).first;
    }
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkClearAttachment clear{};
    clear.aspectMask = t->aspect;
    if (t->aspect == VK_IMAGE_ASPECT_COLOR_BIT) memcpy(&clear.clearValue.color, &c.c, sizeof c.c);
    else clear.clearValue.depthStencil = {c.is_uint ? float(c.c.u[0]) : c.c.f[0],
                                          c.c.u[t->aspect & VK_IMAGE_ASPECT_DEPTH_BIT ? 1 : 0]};
    VkClearRect rect{{{c.r.xoffset, c.r.yoffset},
                      {std::min(unsigned(c.r.width), width - unsigned(c.r.xoffset)),
                       std::min(unsigned(c.r.height), height - unsigned(c.r.yoffset))}}, 0, 1};
    for (unsigned z = 0; z < count; z++) {
        view.min_layer = layer + z;
        VkImageView iv = textureView(*t, &view, true);
        if (!iv) return;
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = found->second.pass; fb.attachmentCount = 1; fb.pAttachments = &iv;
        fb.width = width; fb.height = height; fb.layers = 1;
        VkFramebuffer frame{};
        if (!check(vkCreateFramebuffer(dev, &fb, nullptr, &frame), "texture clear framebuffer")) return;
        cur->framebuffers.push_back(frame);
        VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin.renderPass = found->second.pass; begin.framebuffer = frame;
        begin.renderArea.extent = {width, height};
        vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdClearAttachments(cmd, 1, &clear, 1, &rect);
        vkCmdEndRenderPass(cmd);
    }
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    note("texture clear serial=%llu format=%d view=%d level=%u layers=%u value=%x,%x,%x,%x",
         (unsigned long long)t->serial, int(t->format), int(clearFormat), level, count,
         c.c.u[0], c.c.u[1], c.c.u[2], c.c.u[3]);
    // Keep a tiny GPU readback of initialization clears. This verifies the
    // integer-view bit pattern independently of the subsequent scene draws.
    static unsigned sampledClears{};
    if (t->aspect == VK_IMAGE_ASPECT_COLOR_BIT && sampledClears++ < 8) {
        Slice sample = bytes(nullptr, 16);
        VkBufferImageCopy copy{};
        copy.bufferOffset = sample.off;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, layer, 1};
        copy.imageOffset = {c.r.xoffset, c.r.yoffset, 0};
        copy.imageExtent = {1, 1, 1};
        vkCmdCopyImageToBuffer(cmd, t->image, VK_IMAGE_LAYOUT_GENERAL, sample.b, 1, &copy);
        cur->captures.push_back({"clear_" + std::to_string(t->serial) + ".bin", sample});
    }
}
void copyB2T(const p_copy_b2t &c) {
    endpass();
    TextureCopyPlan plan;
    if (!c.tex || !c.src || !textureCopyPlan(c.tex->d, c.has_view ? &c.view : nullptr, c.r,
                                           S.rowStride, S.imageStride, plan)) return;
    Texture *t = texture(c.tex);
    if (!t || plan.extent > (256u << 20)) return;
    Slice s;
    bool compressed = plan.blockWidth != 1 || plan.blockHeight != 1;
    if (compressed) {
        uint32_t unused; int pixelBytes{};
        if (!nvn_bc_unpacked(c.tex->d.format, &unused, &pixelBytes)) {
            note("unsupported compressed texture upload format=%d", c.tex->d.format); return;
        }
        drainIfOverlap(c.src, plan.extent);
        endpass();
        t = texture(c.tex);
        if (!t) return;
        size_t pitch = size_t(plan.width) * pixelBytes, image = pitch * plan.height;
        if (image > (256u << 20) / plan.count) return;
        std::vector<uint8_t> pixels(image * plan.count);
        for (unsigned z = 0; z < plan.count; z++)
            if (nvn_bc_decode(c.tex->d.format, (const uint8_t *)(uintptr_t)c.src + z * plan.image,
                              plan.row, plan.width, plan.height, pixels.data() + z * image, pitch)) return;
        s = bytes(pixels.data(), pixels.size());
    } else {
        if (plan.row % plan.blockBytes || plan.image % plan.row) {
            note("unsupported texture upload pitch row=%zu image=%zu", plan.row, plan.image); return;
        }
        s = guest(c.src, plan.extent, true);
    }
    VkBufferImageCopy copy{};
    copy.bufferOffset = s.off;
    copy.bufferRowLength = compressed ? 0 : uint32_t(plan.row / plan.blockBytes);
    copy.bufferImageHeight = compressed ? 0 : uint32_t(plan.image / plan.row);
    copy.imageSubresource = {t->aspect, plan.level, plan.volume ? 0 : plan.layer, plan.volume ? 1 : plan.count};
    copy.imageOffset = {c.r.xoffset, c.r.yoffset, plan.volume ? int32_t(plan.layer) : 0};
    copy.imageExtent = {plan.width, plan.height, plan.volume ? plan.count : 1};
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    vkCmdCopyBufferToImage(cmd, s.b, t->image, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
}
bool copyT2B(const p_copy_b2t &c, int64_t rowStride, int64_t imageStride) {
    TextureCopyPlan plan;
    if (!c.tex || !c.src || !textureCopyPlan(c.tex->d, c.has_view ? &c.view : nullptr, c.r,
                                           rowStride, imageStride, plan)) return false;
    if (plan.blockWidth != 1 || plan.blockHeight != 1 || plan.row % plan.blockBytes ||
        plan.image % plan.row || plan.extent > (256u << 20)) {
        note("unsupported texture readback format=%d row=%zu image=%zu", c.tex->d.format, plan.row, plan.image);
        return false;
    }
    endpass();
    drainIfOverlap(c.src, plan.extent);
    endpass();
    Texture *t = texture(c.tex);
    if (!t || (t->aspect & VK_IMAGE_ASPECT_DEPTH_BIT && t->aspect & VK_IMAGE_ASPECT_STENCIL_BIT)) return false;
    // Preserve bytes outside the copied rows, including image/row padding.
    Slice target = bytes((const void *)(uintptr_t)c.src, plan.extent);
    VkBufferImageCopy copy{};
    copy.bufferOffset = target.off;
    copy.bufferRowLength = uint32_t(plan.row / plan.blockBytes);
    copy.bufferImageHeight = uint32_t(plan.image / plan.row);
    copy.imageSubresource = {t->aspect, plan.level, plan.volume ? 0 : plan.layer, plan.volume ? 1 : plan.count};
    copy.imageOffset = {c.r.xoffset, c.r.yoffset, plan.volume ? int32_t(plan.layer) : 0};
    copy.imageExtent = {plan.width, plan.height, plan.volume ? plan.count : 1};
    imageBarrier(*t, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    vkCmdCopyImageToBuffer(cmd, t->image, VK_IMAGE_LAYOUT_GENERAL, target.b, 1, &copy);
    VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 1, &after, 0, nullptr, 0, nullptr);
    recordGpuWrite(c.src, plan.extent);
    invalidateCopies(c.src, plan.extent);
    copies[{c.src, plan.extent}] = target;
    cur->writebacks.push_back({c.src, target, plan.extent});
    return true;
}
void copyT2T(const p_copy_t2t &c) {
    TextureCopyPlan src, dst;
    if (!c.src || !c.dst || !textureCopyPlan(c.src->d, c.has_sv ? &c.sv : nullptr, c.sr, 0, 0, src) ||
        !textureCopyPlan(c.dst->d, c.has_dv ? &c.dv : nullptr, c.dr, 0, 0, dst)) return;
    if (src.count != dst.count) {
        note("unsupported texture copy slice scaling %u -> %u", src.count, dst.count); return;
    }
    endpass();
    Texture *s = texture(c.src), *d = texture(c.dst);
    if (!s || !d || s->aspect != d->aspect) return;
    imageBarrier(*s, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    imageBarrier(*d, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    bool blit = src.width != dst.width || src.height != dst.height || (c.flags & 12) ||
                (s->format != d->format && (!s->texelBytes || s->texelBytes != d->texelBytes));
    std::vector<VkImageCopy> copies;
    std::vector<VkImageBlit> blits;
    for (unsigned z = 0; z < src.count; z++) {
        unsigned sourceZ = c.flags & 16 ? src.count - 1 - z : z;
        for (VkImageAspectFlags aspect : {VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT,
                                         VK_IMAGE_ASPECT_STENCIL_BIT}) {
            if (!(s->aspect & aspect)) continue;
            VkImageSubresourceLayers sl{aspect, src.level, src.volume ? 0 : src.layer + sourceZ, 1};
            VkImageSubresourceLayers dl{aspect, dst.level, dst.volume ? 0 : dst.layer + z, 1};
            VkOffset3D so{c.sr.xoffset, c.sr.yoffset, src.volume ? int32_t(src.layer + sourceZ) : 0};
            VkOffset3D target{c.dr.xoffset, c.dr.yoffset, dst.volume ? int32_t(dst.layer + z) : 0};
            if (blit) {
                VkImageBlit b{};
                b.srcSubresource = sl; b.dstSubresource = dl;
                b.srcOffsets[0] = so;
                b.srcOffsets[1] = {so.x + int32_t(src.width), so.y + int32_t(src.height), so.z + 1};
                if (c.flags & 4) std::swap(b.srcOffsets[0].x, b.srcOffsets[1].x);
                if (c.flags & 8) std::swap(b.srcOffsets[0].y, b.srcOffsets[1].y);
                b.dstOffsets[0] = target;
                b.dstOffsets[1] = {target.x + int32_t(dst.width), target.y + int32_t(dst.height), target.z + 1};
                blits.push_back(b);
            } else copies.push_back({sl, so, dl, target, {src.width, src.height, 1}});
        }
    }
    if (blit)
        vkCmdBlitImage(cmd, s->image, VK_IMAGE_LAYOUT_GENERAL, d->image, VK_IMAGE_LAYOUT_GENERAL,
                       uint32_t(blits.size()), blits.data(),
                       (c.flags & 1) && s->aspect == VK_IMAGE_ASPECT_COLOR_BIT ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
    else vkCmdCopyImage(cmd, s->image, VK_IMAGE_LAYOUT_GENERAL, d->image, VK_IMAGE_LAYOUT_GENERAL,
                        uint32_t(copies.size()), copies.data());
    imageBarrier(*d, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
}
bool copyGpuResult(uint64_t src, uint64_t dst, size_t n) {
    if (!src || !dst || !n || n > (64u << 20)) return false;
    // A GPU-produced source needs an ordered GPU copy, not a memcpy from its
    // deferred guest shadow. Handle subranges (opaque/transparent light bins).
    Slice source{};
    for (auto it = cur->writebacks.rbegin(); it != cur->writebacks.rend(); ++it) {
        if (!overlaps(src, n, it->addr, it->size)) continue;
        if (src < it->addr || src - it->addr > it->size || n > it->size - (src - it->addr))
            return false; // partial producer: synchronize the CPU fallback
        source = it->slice;
        source.off += src - it->addr;
        break;
    }
    if (!source.b) return false;
    endpass();
    recordGpuWrite(dst, n);
    invalidateCopies(dst, n);
    Slice target = stageDyn(nullptr, n);
    VkMemoryBarrier before{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    before.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    before.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 1, &before, 0, nullptr, 0, nullptr);
    VkBufferCopy region{source.off, target.off, n};
    vkCmdCopyBuffer(cmd, source.b, target.b, 1, &region);
    VkMemoryBarrier after{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 1, &after, 0, nullptr, 0, nullptr);
    copies[{dst, n}] = target;
    cur->writebacks.push_back({dst, target, n});
    return true;
}
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
        case OP_CLEAR_TEX:
            clearTexture(*(const p_clear_tex *)a);
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
            if (b.stage >= 0 && b.stage < 6 && b.index >= 0 && b.index < 16) {
                if (h.op == OP_BIND_UBO) S.ub[b.stage][b.index] = b;
                else updateDriverTable(&S.ssbo[b.stage][b.index], &b, sizeof b, b.stage, 0);
            }
            break;
        }
        case OP_BIND_TEX:
        case OP_BIND_SAMP:
        case OP_BIND_IMG: {
            const auto &b = *(const p_bind_handles *)a;
            if (b.stage >= 0 && b.stage < 6 && b.first >= 0 &&
                b.first + b.n <= (h.op == OP_BIND_IMG ? 32 : 128))
                updateDriverTable((h.op == OP_BIND_TEX    ? S.tex[b.stage]
                        : h.op == OP_BIND_SAMP ? S.samp[b.stage]
                                               : S.img[b.stage]) +
                           b.first,
                       b.h, b.n * 8, b.stage, 1);
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
            drainIfOverlap(address, n);
            memcpy((void *)(uintptr_t)address, (const uint8_t *)a + 16, n);
            invalidateCopies(address, n);
            break;
        }
        case OP_COPY_B2B: {
            uint64_t x[3];
            memcpy(x, a, 24);
            if (copyGpuResult(x[0], x[1], x[2])) break;
            drainIfOverlap(x[0], x[2]);
            drainIfOverlap(x[1], x[2]);
            memmove((void *)(uintptr_t)x[1], (void *)(uintptr_t)x[0], x[2]);
            invalidateCopies(x[1], x[2]);
            break;
        }
        case OP_COPY_B2T:
            copyB2T(*(const p_copy_b2t *)a);
            break;
        case OP_COPY_T2B:
            copyT2B(*(const p_copy_b2t *)a, S.rowStride, S.imageStride);
            break;
        case OP_COPY_T2T:
            copyT2T(*(const p_copy_t2t *)a);
            break;
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
            DeferredCounter report{(uint64_t *)x[1], 0, nowNs(), {}};
            if (x[0] == NVN_COUNTER_TYPE_SAMPLES_PASSED) {
                closeQuery();
                report.samples = occlusion.snapshot();
                queryReports++;
            }
            cur->counters.push_back(std::move(report));
            break;
        }
        case OP_RESET_COUNTER: {
            uint64_t t;
            memcpy(&t, a, 8);
            if (t == NVN_COUNTER_TYPE_SAMPLES_PASSED) {
                closeQuery();
                occlusion.reset();
            }
            break;
        }
        case OP_SIGNAL_EVENT: {
            uint32_t *v;
            uint32_t mode, value;
            memcpy(&v, a, 8);
            memcpy(&mode, (const uint8_t *)a + 8, 4);
            memcpy(&value, (const uint8_t *)a + 12, 4);
            cur->events.push_back({v, mode, value});
            break;
        }
        default:
            break;
        }
        p += h.size;
    }
}
// ---------------- completion processing ----------------
// The fence has signaled: make this submission's effects visible (counters,
// events, writebacks) and free everything it owned.
void slotProcess(Slot &s) {
    completedSerial = std::max(completedSerial, s.submitSerial);
    for (auto &c : s.captures) {
        if (FILE *f = fopen((std::string(base) + c.name).c_str(), "wb")) {
            fwrite(c.data.mapped, 1, c.data.size, f); fclose(f);
        }
    }
    s.captures.clear();
    submitLatencyMs += double(nowNs() - s.submittedAtNs) / 1e6;
    for (size_t i = 0; i < s.queries.size(); ++i) {
        auto &q = s.queries[i];
        memcpy(&q->value, queryResults.map + ((&s - ring) * QUERIES_PER_SLOT + i) * 8, 8);
        q->ready = true;
    }
    for (auto &c : s.counters) {
        uint64_t values[2]{c.a + c.samples.value(), c.b};
        if (!c.samples.pending.empty() && values[0] == 0) queryZeros++;
        nvn_dirty_prepare_write(c.ptr, sizeof values);
        memcpy((void *)c.ptr, values, sizeof values);
    }
    s.queries.clear();
    occlusion.compact();

    // Writebacks land BEFORE events: an event is the guest's "you may read the
    // results now" signal, and the reader wakes concurrently with this loop.
    auto wbStart = Clock::now();
    for (auto &w : s.writebacks)
        if (w.slice.b) {
            nvn_dirty_prepare_write((void *)(uintptr_t)w.addr, w.size);
            memcpy((void *)(uintptr_t)w.addr, w.slice.mapped, w.size);
        }
    writebackMs += std::chrono::duration<double, std::milli>(Clock::now() - wbStart).count();
    for (auto &e : s.events) {
        if (!e.ptr)
            continue;
        if (e.mode == 1)
            *e.ptr += e.value;
        else
            *e.ptr = e.value;
    }
    for (auto &t : s.deadTextures) destroyTexture(t);
    s.deadTextures.clear();
    for (auto &b : s.deadBuffers) destroyBuffer(b);
    s.deadBuffers.clear();
    for (auto fb : s.framebuffers)
        vkDestroyFramebuffer(dev, fb, nullptr);
    s.framebuffers.clear();
    for (auto it = retiredTextures.begin(); it != retiredTextures.end() && it->first <= completedSerial;) {
        auto t = textures.find(it->second);
        if (t != textures.end()) {
            invalidateSetsForTexture(it->second, t->second.gen);
            destroyTexture(t->second);
            textures.erase(t);
        }
        it = retiredTextures.erase(it);
    }
    processRetiredSets();
    for (auto it = retiredSamplers.begin(); it != retiredSamplers.end();) {
        if (it->second <= completedSerial) {
            vkDestroySampler(dev, it->first, nullptr);
            it = retiredSamplers.erase(it);
        } else
            ++it;
    }
    invalidateSetsForArena(s);
    vkResetCommandPool(dev, s.pool, 0);
    vkResetFences(dev, 1, &s.fence);
    s.arenaPage = 0;
    s.cursor = 0;
    s.arena = s.arenaPages.empty() ? Buffer{} : s.arenaPages[0];
    s.writebacks.clear();
    s.events.clear();
    s.counters.clear();
    s.live = false;
}
Slot *oldestSlot() {
    Slot *oldest = nullptr;
    for (auto &s : ring)
        if (s.live && (!oldest || s.submitSerial < oldest->submitSerial)) oldest = &s;
    return oldest;
}
void reclaim(Slot &target) {
    if (!target.live) return;
    const auto through = target.submitSerial;
    while (auto *s = oldestSlot()) {
        if (s->submitSerial > through) break;
        auto waitStart = Clock::now();
        if (!check(vkWaitForFences(dev, 1, &s->fence, VK_TRUE, UINT64_MAX), "fence")) return;
        gpuMs += std::chrono::duration<double, std::milli>(Clock::now() - waitStart).count();
        slotProcess(*s);
    }
}
void reclaimSignaled() {
    while (auto *s = oldestSlot()) {
        if (vkGetFenceStatus(dev, s->fence) != VK_SUCCESS) break;
        slotProcess(*s);
    }
}
void reclaimAll() {
    while (auto *s = oldestSlot()) {
        reclaim(*s);
        if (deviceLost) break;
    }
}
// A pending writeback memcpy could clobber a newer CPU write into the same
// guest range; when they overlap, finish all in-flight work first.
bool pendingGuestWrite(const Slot &s, uint64_t addr, uint64_t n) {
    for (auto &w : s.writebacks)
        if (overlaps(addr, n, w.addr, w.size)) return true;
    for (auto &c : s.counters)
        if (overlaps(addr, n, (uint64_t)(uintptr_t)c.ptr, 16)) return true;
    return false;
}
void drainIfOverlap(uint64_t addr, uint64_t n) {
    if (!addr || !n) return;
    // CopyBufferToBuffer / built-in kernels execute on the CPU. A source may
    // have been written by a compute shader EARLIER IN THIS COMMAND BUFFER.
    // Waiting only for older submissions still reads last frame's guest shadow.
    if (cur && !cur->live && pendingGuestWrite(*cur, addr, n)) {
        Slot *submitted = cur;
        finish();
        reclaim(*submitted);
        begin();
        syncDrains++;
        if (syncDrains <= 8)
            RLOG("GPU->CPU copy dependency resolved addr=%{public}llx bytes=%{public}llu",
                 (unsigned long long)addr, (unsigned long long)n);
    }
    for (auto &s : ring)
        if (s.live && pendingGuestWrite(s, addr, n)) {
            reclaimAll();
            syncDrains++;
            return;
        }
}
void readFlags() {
    // Only override the compiled-in default when a flags file is readable;
    // missing files must not clobber the bisect default.
    const std::string appFlags = std::string(base) + "dm_render_flags.txt";
    for (const char *path : {"/data/local/tmp/dm_render_flags.txt", appFlags.c_str()}) {
        if (FILE *config = fopen(path, "r")) {
            unsigned flags = 0;
            if (fscanf(config, "%u", &flags) == 1)
                debugFlags = flags;
            fclose(config);
        }
    }
}
void begin() {
    const std::string captureRequest = std::string(base) + "dm_flash_capture.txt";
    if (FILE *request = fopen(captureRequest.c_str(), "r")) {
        int n = 20;
        if (fscanf(request, "%d", &n) == 1 && n > 0 && n <= 120)
            dmReplayArmCapture(n);
        fclose(request);
        remove(captureRequest.c_str());
    }
    const std::string geometryRequest = std::string(base) + "dm_geometry_capture.txt";
    if (FILE *request = fopen(geometryRequest.c_str(), "r")) {
        int n = 3;
        if (fscanf(request, "%d", &n) == 1 && n > 0 && n <= 3) {
            dmReplayArmCapture(n);
            geometryCapture = true;
        }
        fclose(request);
        remove(geometryRequest.c_str());
    }
    readFlags();
    recordingStart = Clock::now();
    gpuWrites.clear();
    queryCount = 0;
    queryOpen = -1;
    reclaimSignaled();
    Slot &s = ring[curSlot];
    if (s.live)
        reclaim(s);
    cur = &s;
    s.dynStart = dynCursor;
    cmd = s.buffer;
    // slotProcess resets the pool at completion, so the buffer is in its
    // initial state here; an individual reset would need the pool flag.
    VkCommandBufferBeginInfo b{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    b.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd, &b), "begin command");
    graphicsCommands = {};
    if (queryPool)
        vkCmdResetQueryPool(cmd, queryPool, uint32_t((&s - ring) * QUERIES_PER_SLOT), QUERIES_PER_SLOT);
    // Submissions execute in queue order, but Vulkan does not guarantee memory
    // visibility of the previous submission's storage writes (the game's
    // per-frame nvnCommandBufferWaitSync is stubbed out). One full barrier up
    // front replaces what Metal gives each encoder for free.
    VkMemoryBarrier lead{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    lead.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    lead.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1,
                         &lead, 0, nullptr, 0, nullptr);
}
void finish(VkSemaphore wait, VkSemaphore signal) {
    endpass();
    closeQuery();
    if (queryPool && queryCount) {
        vkCmdCopyQueryPoolResults(cmd, queryPool, uint32_t((cur - ring) * QUERIES_PER_SLOT),
                                  uint32_t(queryCount), queryResults.b,
                                  size_t(cur - ring) * QUERIES_PER_SLOT * 8, 8,
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }
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
    auto submitted = Clock::now();
    replayMs += std::chrono::duration<double, std::milli>(submitted - recordingStart).count();
    // Scratch-arena snapshots must never outlive this submission: another slot
    // still executing could reference them after the arena unwinds. Cross-
    // submission reuse goes through resident/persistent buffers only.
    copies.clear();
    dynCopies.clear();
    indexRanges.clear();
    cur->submitSerial = ++submittedSerial;
    serial.store(submittedSerial);
    cur->submittedAtNs = nowNs();
    if (check(vkQueueSubmit(queue, 1, &s, cur->fence), "submit")) {
        cur->live = true;
        curSlot = (curSlot + 1) % RING;
    }
    if (debugFlags & 0x10000)   // synchronous fallback for A/B comparison
        reclaimAll();
}
bool recreateSwapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    if (!check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps), "surface capabilities"))
        return false;
    VkExtent2D nextExtent = caps.currentExtent;
    if (nextExtent.width == UINT32_MAX) nextExtent = extent;
    if (!nextExtent.width || !nextExtent.height) return false;
    reclaimAll();
    if (!check(vkDeviceWaitIdle(dev), "swapchain idle")) return false;
    auto info = swapchainConfig;
    info.oldSwapchain = swapchain;
    info.imageExtent = nextExtent;
    info.minImageCount = std::max(2u, caps.minImageCount);
    if (caps.maxImageCount) info.minImageCount = std::min(info.minImageCount, caps.maxImageCount);
    info.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;
    VkSwapchainKHR replacement{};
    if (!check(vkCreateSwapchainKHR(dev, &info, nullptr, &replacement), "recreate swapchain")) return false;
    vkDestroySwapchainKHR(dev, swapchain, nullptr);
    swapchain = replacement;
    extent = nextExtent;
    swapchainConfig = info;
    swapchainConfig.oldSwapchain = VK_NULL_HANDLE;
    // A failed presentation may leave a binary semaphore signaled. The device
    // is idle, so replace presentation synchronization together with the chain.
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (auto &slot : ring) {
        vkDestroySemaphore(dev, slot.acquireSem, nullptr);
        vkDestroySemaphore(dev, slot.renderSem, nullptr);
        slot.acquireSem = slot.renderSem = VK_NULL_HANDLE;
        if (!check(vkCreateSemaphore(dev, &sem, nullptr, &slot.acquireSem), "recreate acquire semaphore") ||
            !check(vkCreateSemaphore(dev, &sem, nullptr, &slot.renderSem), "recreate render semaphore")) {
            deviceLost = true;
            return false;
        }
    }
    swapchainNeedsRebuild = false;
    note("swapchain recovered %ux%u frame=%llu", extent.width, extent.height,
         (unsigned long long)frames.load());
    RLOG("swapchain recovered %{public}ux%{public}u", extent.width, extent.height);
    return true;
}
void savePipelineCache() {
    static size_t savedPipelines = 0;
    static uint64_t savedAt = 0;
    if (!pipelineCache || pipelines.size() == savedPipelines || nowNs() - savedAt < 5000000000ull)
        return;
    savedAt = nowNs();
    size_t n = 0;
    if (vkGetPipelineCacheData(dev, pipelineCache, &n, nullptr) || !n || n > (96u << 20))
        return;
    std::vector<uint8_t> data(n);
    if (vkGetPipelineCacheData(dev, pipelineCache, &n, data.data()))
        return;
    std::string path = std::string(base) + "dm_pipeline_cache.bin";
    FILE *f = fopen((path + ".tmp").c_str(), "wb");
    if (!f)
        return;
    fwrite(data.data(), 1, n, f);
    fclose(f);
    if (rename((path + ".tmp").c_str(), path.c_str()) == 0)
        savedPipelines = pipelines.size();
}
void stats() {
    FILE *f = fopen((std::string(base) + "dm_replay_stats.txt").c_str(), "w");
    if (f) {
        fprintf(f,
                "submit=%llu present=%llu draws=%llu dispatch=%llu upload=%llu shaders=%zu pipelines=%zu "
                "pipeline_errors=%llu missing_shader=%llu textures=%zu arena=%zu "
                "replay_ms=%.1f gpu_ms=%.1f writeback_ms=%.1f latency_ms=%.1f "
                "descriptor_ms=%.1f staging_ms=%.1f index_ms=%.1f staged_mb=%.1f "
                "resident_mb=%.1f resident_hits=%llu "
                "set_alloc=%llu set_hits=%llu set_invalid=%llu bank_hits=%llu bank_builds=%llu sync_drains=%llu debug_flags=%u retired_sets=%zu\n",
                (unsigned long long)submits, (unsigned long long)frames.load(), (unsigned long long)draws,
                (unsigned long long)dispatches, (unsigned long long)uploads, shaders.size(), pipelines.size(),
                (unsigned long long)pipelineErrors, (unsigned long long)missing, textures.size(),
                cur ? cur->cursor : 0,
                replayMs, gpuMs, writebackMs, submitLatencyMs, descriptorMs, stagingMs, indexMs,
                double(stagedBytes) / (1024 * 1024), double(residentBytes) / (1024 * 1024),
                (unsigned long long)residentHits,
                (unsigned long long)setAllocs, (unsigned long long)setHits, (unsigned long long)setInvalidations,
                (unsigned long long)bankHits, (unsigned long long)bankBuilds, (unsigned long long)syncDrains,
                debugFlags, retiredSets.size());
        fclose(f);
    }
    static uint64_t lastAt = 0, lastFrames = 0, lastDraws = 0, lastPasses = 0;
    static uint64_t lastStateCommands = 0, lastStateSkipped = 0;
    static double lastReplay = 0, lastGpu = 0, lastDesc = 0, lastStage = 0, lastIndex = 0;
    const auto at = nowNs();
    if (lastAt && at - lastAt >= 1000000000ull) {
        const auto n = frames.load() - lastFrames;
        const double divisor = double(std::max(uint64_t(1), n));
        RLOG("PERF fps=%{public}.1f draws/f=%{public}.0f passes/f=%{public}.0f cpu/f=%{public}.1f wait/f=%{public}.1f desc/f=%{public}.1f stage/f=%{public}.1f index/f=%{public}.1f queries=%{public}llu zero=%{public}llu fallback=%{public}llu flags=%{public}u state/f=%{public}.0f skipped/f=%{public}.0f phase_sample=32",
             double(n)*1e9/double(at-lastAt), double(draws-lastDraws)/divisor,
             double(passCount-lastPasses)/divisor, (replayMs-lastReplay)/divisor,
             (gpuMs-lastGpu)/divisor, (descriptorMs-lastDesc)/divisor,
             (stagingMs-lastStage)/divisor, (indexMs-lastIndex)/divisor,
             (unsigned long long)queryReports, (unsigned long long)queryZeros,
             (unsigned long long)queryFallbacks, debugFlags,
             double(graphicsStateCommands-lastStateCommands)/divisor,
             double(graphicsStateSkipped-lastStateSkipped)/divisor);
    }
    if (!lastAt || at-lastAt >= 1000000000ull) {
        lastAt = at; lastFrames = frames.load(); lastDraws = draws; lastPasses = passCount;
        lastReplay = replayMs; lastGpu = gpuMs; lastDesc = descriptorMs;
        lastStage = stagingMs; lastIndex = indexMs;
        lastStateCommands = graphicsStateCommands; lastStateSkipped = graphicsStateSkipped;
    }
    savePipelineCache();
}
// Applies completions promptly even while the game thread is between submits
// (e.g. spinning on a guest event between command batches).
std::atomic<bool> completionRun{false};
void completionLoop() {
    while (completionRun.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        std::lock_guard<std::recursive_mutex> lock(mu);
        if (ready && !deviceLost)
            reclaimSignaled();
    }
}
} // namespace
extern "C" uint64_t dmReplayFrames() { return frames.load(); }
extern "C" uint64_t dmReplaySerial() { return serial.load(); }
extern "C" void dmReplayArmCapture(int n) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    capturedFrames = 0;
    armedCapture.store(n);
    mkdir((std::string(base) + "flash").c_str(), 0755);
    if (captureTrace) fclose(captureTrace);
    captureBuffers.clear();
    captureSequence = 0;
    geometryCapture = false;
    geometryCaptureBytes = 0;
    geometrySlices.clear();
    captureTrace = fopen((std::string(base) + "flash/draws.tsv").c_str(), "w");
    if (captureTrace) {
        fprintf(captureTrace, "CAPTURE_START frame=%llu draws=%llu serial=%llu\n",
                (unsigned long long)frames.load(), (unsigned long long)draws,
                (unsigned long long)serial.load());
        fprintf(captureTrace, "FALLBACK_DRAW_MASK %x\n", dmNvnFallbackDrawMask());
        dmNvnTraceFallbacks(captureTrace);
        fflush(captureTrace);
    }
}
extern "C" void dmReplaySubmit(int n, const uint64_t *h) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!ready || deviceLost)
        return;
    begin();
    for (int i = 0; i < n; i++)
        execute((const uint8_t *)(uintptr_t)h[i]);
    finish();
    submits++;
    if (submits % 30 == 0)
        stats();
}
extern "C" void dmReplayPresent(const tex_t *t, const int *crop) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!ready || !t || deviceLost)
        return;
    // Acquire before recording. On an out-of-date chain no semaphore is
    // signaled and no command buffer may be left open for the next submit.
    if (swapchainNeedsRebuild && !recreateSwapchain()) return;
    auto &slot = ring[curSlot];
    if (slot.live) reclaim(slot);
    uint32_t index{};
    VkResult r = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, slot.acquireSem, VK_NULL_HANDLE, &index);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchainNeedsRebuild = true;
        if (!recreateSwapchain()) return;
        r = vkAcquireNextImageKHR(dev, swapchain, UINT64_MAX, slot.acquireSem, VK_NULL_HANDLE, &index);
    }
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        if (r == VK_ERROR_OUT_OF_DATE_KHR) swapchainNeedsRebuild = true;
        check(r, "acquire");
        return;
    }
    if (r == VK_SUBOPTIMAL_KHR) swapchainNeedsRebuild = true;
    VkSemaphore acquireSem = slot.acquireSem, renderSem = slot.renderSem;
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
    bool flashCapture = armedCapture.load() > 0;
    if (flashCapture)
        mkdir((std::string(base) + "flash").c_str(), 0755);
    Slice capture{};
    if (src && (flashCapture || frames == 1 || frames == 60 || frames == 1800) &&
        src->aspect == VK_IMAGE_ASPECT_COLOR_BIT &&
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
    Slot *submitted = cur;
    bool legacyPresent = debugFlags & 128;
    finish(acquireSem, renderSem);
    if (legacyPresent || !dumps.empty() || capture.b)
        reclaim(*submitted);   // legacy mode: blit complete before present; debug readbacks need the bytes
    for (auto &dump : dumps) {
        FILE *f = fopen((std::string(base)+"capture_"+std::to_string(dump.id)+".bin").c_str(),"wb");
        if(f) {
            uint32_t header[]{dump.texture->w,dump.texture->h,dump.texture->depth,(uint32_t)dump.texture->format};
            fwrite(header,1,sizeof header,f); fwrite(dump.data.mapped,1,dump.data.size,f);fclose(f);
        }
    }
    if (capture.b && flashCapture && armedCapture.fetch_sub(1) > 0) {
        char name[96];
        snprintf(name, sizeof name, "flash/f%llu.ppm", (unsigned long long)capturedFrames++);
        FILE *f = fopen((std::string(base) + name).c_str(), "wb");
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
    } else if (capture.b) {
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
    pi.pWaitSemaphores = &renderSem;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;
    pi.pImageIndices = &index;
    VkResult presented = vkQueuePresentKHR(queue, &pi);
    if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR)
        swapchainNeedsRebuild = true;
    else check(presented, "present");
    if (debugFlags & 0x10000 || legacyPresent)   // synchronous fallback for A/B comparison
        vkQueueWaitIdle(queue);
    if (captureTrace) {
        fflush(captureTrace);
        if (armedCapture.load() <= 0) {
            fprintf(captureTrace, "CAPTURE_DONE frame=%llu draws=%llu\n",
                    (unsigned long long)frames.load(), (unsigned long long)draws);
            fclose(captureTrace); captureTrace = nullptr;
        }
    }
    frames++;
    if (frames % 10 == 0)
        stats();
}
extern "C" void mtl_pool_add(void *p, size_t n) { nvn_dirty_add_range(p,n); }
extern "C" void mtl_pool_del(void *p) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!ready)
        return;
    // Pending writebacks memcpy into pool memory; the old synchronous design
    // guaranteed they were applied before pool destruction. Keep that guarantee.
    reclaimAll();
    // A newly allocated pool may reuse both address and zero generations.
    // Pool destruction therefore invalidates resident snapshots and every
    // address-keyed snapshot: a same-submit reallocation at the same address
    // must never hit the dead pool's bytes (area transitions, text atlases).
    copies.clear();
    dynCopies.clear();
    indexRanges.clear();
    for (auto it = residentBuffers.begin(); it != residentBuffers.end();) {
        for (auto c = copies.begin(); c != copies.end();)
            c = c->second.b == it->second.buffer.b ? copies.erase(c) : std::next(c);
        invalidateSetsForBuffer(it->second.buffer.b);
        destroyBuffer(it->second.buffer);
        residentBytes -= it->second.buffer.size;
        it = residentBuffers.erase(it);
    }
    nvn_dirty_remove_range(p);
}
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
        // Cached sets referencing the old sampler object must be rebuilt; the
        // object itself is retired once the GPU passes the current serial.
        uint32_t gen = samplerGen[id]++;
        for (auto &[k, sh] : shaders)
            for (size_t i = 0; i < sh.entries.size(); i++)
                if (!sh.entries[i].dead &&
                    std::find(sh.entries[i].refSamplers.begin(), sh.entries[i].refSamplers.end(),
                              std::make_pair(id, gen)) != sh.entries[i].refSamplers.end())
                    retireSet(sh, i);
        retiredSamplers.push_back({it->second, submittedSerial + 1});
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
    if (ready && !deviceLost) {
        reclaimAll();
        vkQueueWaitIdle(queue);
    }
}
// A fenced sync releases only after the GPU completed that submission AND its
// guest-visible effects (writebacks, events) were applied.
extern "C" int mtl_wait(uint64_t s, uint64_t timeout) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (completedSerial >= s)
        return 1;
    if (!ready || deviceLost)
        return completedSerial >= s;
    auto deadline = timeout == UINT64_MAX ? Clock::time_point::max()
                                          : Clock::now() + std::chrono::nanoseconds(timeout);
    while (completedSerial < s) {
        Slot *oldest = nullptr;
        for (auto &slot : ring)
            if (slot.live && (!oldest || slot.submitSerial < oldest->submitSerial))
                oldest = &slot;
        if (!oldest)
            break;
        uint64_t leftNs = UINT64_MAX;
        if (deadline != Clock::time_point::max()) {
            auto left = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - Clock::now()).count();
            if (left <= 0)
                return 0;
            leftNs = uint64_t(left);
        }
        auto waitStart = Clock::now();
        VkResult r = vkWaitForFences(dev, 1, &oldest->fence, VK_TRUE, leftNs);
        gpuMs += std::chrono::duration<double, std::milli>(Clock::now() - waitStart).count();
        if (r == VK_TIMEOUT) return 0;
        if (!check(r, "guest fence wait")) return 0;
        slotProcess(*oldest);
    }
    return completedSerial >= s;
}
extern "C" void mtl_read_texels(const tex_t *t, unsigned level, unsigned layer, const NVNcopyRegion *r,
                                uint8_t *dst, size_t row, size_t img) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    if (!t || !dst || !r)
        return;
    if (row > INT64_MAX || img > INT64_MAX) return;
    view_t view{};
    view.set_levels = view.set_layers = 1;
    view.base_level = int(level); view.min_layer = int(layer);
    view.nlevels = 1;
    TextureCopyPlan plan;
    if (!textureCopyPlan(t->d, &view, *r, int64_t(row), int64_t(img), plan)) return;
    // Packaged assets are read from Tegra storage; render targets must be read
    // from the GPU after their producers complete, not from the stale shadow.
    if (ready && !deviceLost)
        reclaimAll();
    if (readPackagedTexture(*t, plan, *r, dst)) return;
    if (!ready || deviceLost) return;
    p_copy_b2t request{};
    request.tex = t; request.src = uint64_t(uintptr_t(dst));
    request.view = view; request.has_view = 1; request.r = *r;
    begin();
    bool copied = copyT2B(request, int64_t(row), int64_t(img));
    Slot *submitted = cur;
    finish();
    if (copied) reclaim(*submitted);
}
static void replayCleanup() {
    if (dev)
        vkDeviceWaitIdle(dev);
    ready = false;
    for (auto &s : shaders)
        for (auto &e : s.second.entries)
            for (auto v : e.views)
                vkDestroyBufferView(dev, v, nullptr);
    for (auto &r : retiredSets) {
        if (r.set)
            vkFreeDescriptorSets(dev, setpool, 1, &r.set);
        for (auto v : r.views)
            vkDestroyBufferView(dev, v, nullptr);
    }
    retiredSets.clear();
    for (auto &s : retiredSamplers)
        vkDestroySampler(dev, s.first, nullptr);
    retiredSamplers.clear();
    for (auto &[key, r] : residentBuffers) destroyBuffer(r.buffer);
    residentBuffers.clear();
    residentBytes = 0;
    for (auto &[k, p] : pipelines) {
        if (p.pipe)
            vkDestroyPipeline(dev, p.pipe, nullptr);
        if (p.layout)
            vkDestroyPipelineLayout(dev, p.layout, nullptr);
    }
    pipelines.clear();
    if (waterDebugModule) vkDestroyShaderModule(dev, waterDebugModule, nullptr);
    waterDebugModule = {};
    if (pipelineCache) {
        vkDestroyPipelineCache(dev, pipelineCache, nullptr);
        pipelineCache = {};
    }
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
    retiredTextures.clear();
    for (auto &[k, s] : samplers)
        vkDestroySampler(dev, s, nullptr);
    samplers.clear();
    for (auto &page : persistentPages) {
        persistentArena = page;
        vkUnmapMemory(dev, persistentArena.mem);
        vkDestroyBuffer(dev, persistentArena.b, nullptr);
        vkFreeMemory(dev, persistentArena.mem, nullptr);
    }
    persistentPages.clear();
    persistentArena = {};
    persistentCursor = 0;
    bankCache.clear();
    for (auto &stage : lastDriverBank)
        for (auto &bank : stage) bank = {};
    occlusion = {};
    copies.clear();
    dynCopies.clear();
    indexRanges.clear();
    if (dynRing.b) {
        vkUnmapMemory(dev, dynRing.mem);
        vkDestroyBuffer(dev, dynRing.b, nullptr);
        vkFreeMemory(dev, dynRing.mem, nullptr);
        dynRing = {};
        dynCursor = 0;
    }
    if (queryPool) {
        vkDestroyQueryPool(dev, queryPool, nullptr);
        queryPool = {};
    }
    if (queryResults.b)
        destroyBuffer(queryResults);
    for (auto &s : ring) {
        for (auto &page : s.arenaPages) {
            s.arena = page;
            vkUnmapMemory(dev, s.arena.mem);
            vkDestroyBuffer(dev, s.arena.b, nullptr);
            vkFreeMemory(dev, s.arena.mem, nullptr);
        }
        s.arenaPages.clear();
        s.arena = {};
        s.cursor = s.arenaPage = 0;
        s.live = false;
        for (auto &t : s.deadTextures) destroyTexture(t);
        s.deadTextures.clear();
        for (auto &b : s.deadBuffers) destroyBuffer(b);
        s.deadBuffers.clear();
        if (s.pool)
            vkDestroyCommandPool(dev, s.pool, nullptr);
        if (s.fence)
            vkDestroyFence(dev, s.fence, nullptr);
        if (s.acquireSem)
            vkDestroySemaphore(dev, s.acquireSem, nullptr);
        if (s.renderSem)
            vkDestroySemaphore(dev, s.renderSem, nullptr);
        s.pool = {};
        s.fence = {};
        s.acquireSem = s.renderSem = {};
        s.buffer = {};
    }
    curSlot = 0;
    cur = {};
    cmd = {};
    if (setpool)
        vkDestroyDescriptorPool(dev, setpool, nullptr);
    setpool = {};
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
    swapchainConfig = {};
    swapchainNeedsRebuild = false;
    submittedSerial = completedSerial = 0;
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
        swapchainConfig = sw;
        VkSemaphoreCreateInfo sm{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        for (auto &s : ring) {
            VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pc.queueFamilyIndex = qfamily;
            check(vkCreateCommandPool(dev, &pc, nullptr, &s.pool), "command pool");
            VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            ca.commandPool = s.pool;
            ca.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ca.commandBufferCount = 1;
            check(vkAllocateCommandBuffers(dev, &ca, &s.buffer), "command buffer");
            check(vkCreateFence(dev, &fc, nullptr, &s.fence), "fence");
            check(vkCreateSemaphore(dev, &sm, nullptr, &s.acquireSem), "semaphore");
            check(vkCreateSemaphore(dev, &sm, nullptr, &s.renderSem), "semaphore");
        }
        cur = &ring[0];
        cmd = ring[0].buffer;
        dynAlign = 256;
        for (auto limit : {properties.limits.minUniformBufferOffsetAlignment,
                           properties.limits.minStorageBufferOffsetAlignment})
            while (dynAlign < limit)
                dynAlign <<= 1;
        note("dyn ring align=%zu maxDynamicU=%u maxDynamicS=%zu", dynAlign,
             properties.limits.maxDescriptorSetUniformBuffersDynamic,
             size_t(properties.limits.maxDescriptorSetStorageBuffersDynamic));
        {
            VkPipelineCacheCreateInfo pci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            auto blob = read(std::string(base) + "dm_pipeline_cache.bin");
            if (!blob.empty()) {
                pci.initialDataSize = blob.size();
                pci.pInitialData = blob.data();
            }
            check(vkCreatePipelineCache(dev, &pci, nullptr, &pipelineCache), "pipeline cache");
        }
        VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 131072},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 131072},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32768},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 131072},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 131072},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16384},
                                        {VK_DESCRIPTOR_TYPE_SAMPLER, 32768},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32768},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32768}};
        VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        dp.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        dp.maxSets = 32768;
        dp.poolSizeCount = 10;
        dp.pPoolSizes = sizes;
        check(vkCreateDescriptorPool(dev, &dp, nullptr, &setpool), "descriptor pool");
        {
            VkQueryPoolCreateInfo qc{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
            qc.queryType = VK_QUERY_TYPE_OCCLUSION;
            qc.queryCount = RING * QUERIES_PER_SLOT;
            if (!vkCreateQueryPool(dev, &qc, nullptr, &queryPool)) {
                queryResults = buffer(RING * QUERIES_PER_SLOT * 8, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
                note("occlusion queries enabled %u slots", qc.queryCount);
            } else
                note("occlusion query pool unavailable; visibility uses conservative fallback");
        }
        persistentArena = {};   // first persistent() call allocates page 0
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
    completionRun = true;
    std::thread completion(completionLoop);
    while (*running)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    completionRun = false;
    completion.join();
    std::lock_guard<std::recursive_mutex> lock(mu);
    stats();
    replayCleanup();
}
