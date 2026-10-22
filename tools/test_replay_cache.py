"""Exercise the replay cache's actual range invalidation without a GPU."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ReplayCacheTest(unittest.TestCase):
    def test_present_recovers_outdated_swapchain_before_recording(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('    // Acquire before recording.')
        end = source.index('    Texture *src = texture(t);', start)
        code = r'''
#include <cassert>
#include <cstdint>
#include <deque>
using VkResult = int;
using VkSemaphore = int;
constexpr int VK_SUCCESS=0, VK_SUBOPTIMAL_KHR=1, VK_ERROR_OUT_OF_DATE_KHR=-1;
constexpr int VK_NULL_HANDLE=0;
struct Slot { bool live{}; int acquireSem{10},renderSem{20}; } ring[2];
int curSlot{}, dev{}, swapchain{}, begins{}, rebuilds{}, reclaims{}, checks{}, boundAcquire{}, boundRender{};
bool swapchainNeedsRebuild{}, rebuildSucceeds=true;
std::deque<int> results;
void reclaim(Slot &s) { reclaims++; s.live=false; }
bool recreateSwapchain() {
    rebuilds++;
    if (!rebuildSucceeds) return false;
    for (auto &s:ring) { s.acquireSem+=100; s.renderSem+=100; }
    swapchainNeedsRebuild=false;
    return true;
}
VkResult vkAcquireNextImageKHR(int,int,uint64_t,int semaphore,int,uint32_t *index) {
    assert(begins==0); // acquiring and recovery must precede command recording
    assert(semaphore==ring[curSlot].acquireSem);
    assert(!results.empty()); int r=results.front(); results.pop_front(); *index=3; return r;
}
void begin() { begins++; }
void check(int,const char*) { checks++; }
void present() {
''' + source[start:end] + r'''
    boundAcquire=acquireSem; boundRender=renderSem;
}
void reset() {
    ring[0]={}; begins=rebuilds=reclaims=checks=boundAcquire=boundRender=0;
    swapchainNeedsRebuild=false; rebuildSucceeds=true; results.clear();
}
int main() {
    reset(); ring[0].live=true; results={VK_SUCCESS}; present();
    assert(begins==1 && reclaims==1 && rebuilds==0 && boundAcquire==10);
    reset(); results={VK_ERROR_OUT_OF_DATE_KHR,VK_SUCCESS}; present();
    assert(begins==1 && rebuilds==1 && !swapchainNeedsRebuild);
    assert(boundAcquire==110 && boundRender==120); // use replacement semaphores
    reset(); results={VK_ERROR_OUT_OF_DATE_KHR,VK_ERROR_OUT_OF_DATE_KHR}; present();
    assert(begins==0 && rebuilds==1 && swapchainNeedsRebuild && checks==1);
    reset(); rebuildSucceeds=false; results={VK_ERROR_OUT_OF_DATE_KHR}; present();
    assert(begins==0 && rebuilds==1 && swapchainNeedsRebuild);
    reset(); swapchainNeedsRebuild=true; results={VK_SUCCESS}; present();
    assert(begins==1 && rebuilds==1 && boundAcquire==110);
    reset(); results={VK_SUBOPTIMAL_KHR}; present();
    assert(begins==1 && swapchainNeedsRebuild && rebuilds==0);
    reset(); results={-9}; present();
    assert(begins==0 && checks==1 && rebuilds==0);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-present-') as tmp:
            folder = pathlib.Path(tmp)
            path = folder / 'main.cpp'
            path.write_text(code)
            exe = folder / 'present-test'
            subprocess.run([compiler, '-std=c++17', '-O2', str(path), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_graphics_commands_change_and_command_buffer_reset(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('struct GraphicsCommands {')
        end = source.index('void draw(', start)
        code = r'''
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
using VkPipeline = int;
struct VkViewport { float x,y,w,h,n,f; };
struct VkRect2D { struct { int x,y; } offset; struct { uint32_t w,h; } extent; };
enum { VK_PIPELINE_BIND_POINT_GRAPHICS, VK_STENCIL_FACE_FRONT_AND_BACK };
int cmd{}, calls[7]{};
template<class... T> void vkCmdBindPipeline(T...) { calls[0]++; }
template<class... T> void vkCmdSetViewport(T...) { calls[1]++; }
template<class... T> void vkCmdSetScissor(T...) { calls[2]++; }
template<class... T> void vkCmdSetBlendConstants(T...) { calls[3]++; }
template<class... T> void vkCmdSetStencilReference(T...) { calls[4]++; }
template<class... T> void vkCmdSetStencilCompareMask(T...) { calls[5]++; }
template<class... T> void vkCmdSetStencilWriteMask(T...) { calls[6]++; }
uint32_t passW=800,passH=600;
struct {
    float viewport[4]{},nearZ{},farZ{1},blendColor[4]{};
    struct { int x{},y{},w{},h{}; } sc;
    int stencilRef{},stencilValueMask{255},stencilMask{255};
} S;
''' + source[start:end] + r'''
int main() {
    bindGraphicsPipeline(7); dynamic();
    bindGraphicsPipeline(7); dynamic();
    for (int c : calls) assert(c==1);
    S.viewport[0]=4; dynamic(); assert(calls[1]==2 && calls[2]==1);
    S.sc.w=123; dynamic(); assert(calls[2]==2);
    S.blendColor[3]=1; dynamic(); assert(calls[3]==2);
    S.stencilRef=2; dynamic(); assert(calls[4]==2);
    S.stencilValueMask=15; dynamic(); assert(calls[5]==2);
    S.stencilMask=15; dynamic(); assert(calls[6]==2);
    bindGraphicsPipeline(8); dynamic(); assert(calls[0]==2);
    for (int c : calls) assert(c==2);
    graphicsCommands={}; // a new command buffer inherits no graphics state
    bindGraphicsPipeline(8); dynamic();
    for (int c : calls) assert(c==3);
    passH=480; dynamic(); assert(calls[1]==4 && calls[2]==4);
    assert(graphicsStateSkipped>graphicsStateCommands);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-graphics-state-') as tmp:
            cpp = pathlib.Path(tmp) / 'state.cpp'
            exe = pathlib.Path(tmp) / 'state-test'
            cpp.write_text(code)
            subprocess.run([compiler, '-std=c++17', '-O2', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_driver_bank_invalidates_only_changed_table(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('struct DriverBankLast {')
        end = source.index('Slice bindingBuffer(', start)
        code = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
struct Slice { int b{}; };
''' + source[start:end] + r'''
int main() {
    uint64_t dst[4]{}, src[4]{0, 19, 0, 0};
    for (auto &stage : lastDriverBank)
        for (auto &bank : stage) bank.valid = true;
    updateDriverTable(dst, src, sizeof dst, 1, 1);
    assert(dst[1] == 19 && !lastDriverBank[1][1].valid);
    assert(lastDriverBank[1][0].valid && lastDriverBank[0][1].valid);
    lastDriverBank[1][1].valid = true;
    updateDriverTable(dst, src, sizeof dst, 1, 1);
    assert(lastDriverBank[1][1].valid);
    src[1] = 22;
    updateDriverTable(dst, src, sizeof dst, 1, 0);
    assert(!lastDriverBank[1][0].valid && lastDriverBank[1][1].valid);
    assert(dst[1] == 22);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-driver-bank-') as tmp:
            cpp = pathlib.Path(tmp) / 'bank.cpp'
            exe = pathlib.Path(tmp) / 'bank-test'
            cpp.write_text(code)
            subprocess.run([compiler, '-std=c++17', '-O2', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_storage_alias_across_descriptor_modes(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('Slice guest(uint64_t addr,')
        end = source.index('struct Registration', start)
        code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
struct Slice { int b{}; size_t off{}, size{}; uint8_t *mapped{}; };
std::map<std::pair<uint64_t, uint64_t>, Slice> copies, dynCopies;
std::vector<std::unique_ptr<uint8_t[]>> allocations;
Slice zeroSlice;
unsigned debugFlags{};
#define RLOG(...) ((void)0)
struct Writeback { uint64_t addr; Slice slice; size_t size; };
struct Slot { std::vector<Writeback> writebacks; } slot;
Slot *cur = &slot;
size_t dynAlign = 256;
bool overlaps(uint64_t a,uint64_t n,uint64_t b,uint64_t m) { return a<=b ? b-a<n : a-b<m; }
Slice stageDyn(const void *p, size_t n) {
    auto memory = std::make_unique<uint8_t[]>(n);
    if (p) memcpy(memory.get(), p, n);
    Slice s{int(allocations.size()+1), 0, n, memory.get()};
    allocations.push_back(std::move(memory));
    return s;
}
Slice bytes(const void *p, size_t n) { return stageDyn(p,n); }
Slice persistent(const void *p, size_t n) { return stageDyn(p,n); }
Slice resident(uint64_t, size_t) { return {}; }
enum { VK_STRUCTURE_TYPE_MEMORY_BARRIER, VK_ACCESS_MEMORY_WRITE_BIT,
       VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT };
struct VkMemoryBarrier { int type{}, srcAccessMask{}, dstAccessMask{}; };
struct VkBufferCopy { size_t srcOffset, dstOffset, size; };
int cmd{};
void endpass() {}
template<class... T> void vkCmdPipelineBarrier(T...) {}
void vkCmdCopyBuffer(int,int src,int dst,int,const VkBufferCopy *r) {
    memcpy(allocations[dst-1].get()+r->dstOffset,allocations[src-1].get()+r->srcOffset,r->size);
}
''' + source[start:end] + r'''
int main() {
    uint8_t memory[4096]{};
    const auto a = uint64_t(uintptr_t(memory));
    auto compute = guest(a, sizeof memory, false, false, false);
    compute.mapped[12] = 123; // emulate a GPU storage write; CPU remains stale
    auto fragment = guest(a, sizeof memory, true, false, true);
    assert(fragment.b == compute.b && fragment.mapped[12] == 123);
    auto prefix = guest(a, 1024, false, false, true);
    assert(prefix.b == compute.b && prefix.size == 1024);
    prefix.mapped[8] = 45;
    assert(guest(a, sizeof memory, true, false, false).mapped[8] == 45);
    assert(memory[12] == 0 && memory[8] == 0);
    // Compute writes a double-sized bin array; fragment shaders select half
    // with a different base address. Their data must stay on the GPU slice.
    compute.mapped[2048 + 12] = 77;
    slot.writebacks.push_back({a,compute,sizeof memory});
    auto half = guest(a+2048,2048,true,false,true);
    assert(half.b==compute.b && half.off==compute.off+2048);
    assert(half.mapped[12]==77 && memory[2048+12]==0);
    auto unaligned = guest(a+4,1024,true,false,true,256);
    assert(unaligned.b!=compute.b && unaligned.off%256==0);
    assert(unaligned.mapped[8]==123 && memory[12]==0);
    slot.writebacks.clear();
    copies.clear(); // a new submit takes a fresh CPU snapshot
    assert(guest(a, sizeof memory, true, false, true).mapped[12] == 0);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-alias-') as tmp:
            cpp = pathlib.Path(tmp) / 'alias.cpp'
            exe = pathlib.Path(tmp) / 'alias-test'
            cpp.write_text(code)
            subprocess.run([compiler, '-std=c++17', '-O2', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_cpu_copy_observes_current_gpu_write(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('bool pendingGuestWrite(')
        end = source.index('void readFlags()', start)
        code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#define RLOG(...) ((void)0)
struct Write { uint64_t addr, size; };
struct Counter { uint64_t *ptr; };
struct Slot { bool live{}; std::vector<Write> writebacks; std::vector<Counter> counters; };
Slot ring[2], *cur = &ring[0];
uint64_t syncDrains{};
unsigned finishes{}, begins{};
uint8_t memory[64]{};
bool overlaps(uint64_t a,uint64_t n,uint64_t b,uint64_t m) { return a<=b ? b-a<n : a-b<m; }
void finish() { finishes++; cur->live=true; }
void reclaim(Slot &s) { memory[8]=42; s.live=false; s.writebacks.clear(); s.counters.clear(); }
void reclaimAll() { for(auto &s:ring)if(s.live)reclaim(s); }
void begin() { begins++; cur=&ring[1]; }
''' + source[start:end] + r'''
int main() {
    auto address=uint64_t(uintptr_t(memory));
    cur->writebacks.push_back({address,32});
    drainIfOverlap(address+32,1); // touching boundary does not wait
    assert(!finishes);
    drainIfOverlap(address+8,1);
    uint8_t copied; memcpy(&copied,memory+8,1);
    assert(copied==42 && finishes==1 && begins==1 && syncDrains==1);
    ring[0].live=true; ring[0].writebacks.push_back({address,32});
    memory[8]=0; drainIfOverlap(address+8,1);
    assert(memory[8]==42 && finishes==1 && syncDrains==2);
    cur->counters.push_back({reinterpret_cast<uint64_t *>(memory)});
    drainIfOverlap(address,16); // CPU built-in query kernels also need the report
    assert(finishes==2 && begins==2);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-copy-') as tmp:
            cpp = pathlib.Path(tmp) / 'copy.cpp'
            exe = pathlib.Path(tmp) / 'copy-test'
            cpp.write_text(code)
            subprocess.run([compiler, '-std=c++17', '-O2', str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_counter_kernel_forwarding(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('bool counterWord(')
        end = source.index('bool builtinDispatch()', start)
        code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include "gfx/nvn/occlusion_counter.h"
struct Slice { uint8_t *mapped{}; };
struct Writeback { uint64_t addr; Slice slice; size_t size; bool hostReady; };
struct Counter { uint64_t *ptr; uint64_t a,b; OcclusionSnapshot samples; };
struct Slot { bool live{}; uint64_t submitSerial{}; std::vector<Writeback> writebacks; std::vector<Counter> counters; };
Slot ring[2], *cur=&ring[1];
unsigned waits{};
bool overlaps(uint64_t a,uint64_t n,uint64_t b,uint64_t m) { return a<=b ? b-a<n : a-b<m; }
void drainIfOverlap(uint64_t address,uint64_t) {
    waits++; uint64_t value=77; memcpy((void *)(uintptr_t)address,&value,8);
    cur->counters.clear(); cur->writebacks.clear();
}
''' + source[start:end] + r'''
int main() {
    uint64_t shadow[4]{11,22,33,44}, ready=99;
    auto address=uint64_t(uintptr_t(shadow));
    cur->counters.push_back({shadow,0,9000,{}});
    assert(cpuKernelWord(address)==0 && cpuKernelWord(address+8)==9000);
    assert(!waits && shadow[0]==11 && shadow[1]==22); // still not guest-visible
    cur->counters.clear();
    cur->writebacks.push_back({address,{(uint8_t *)&ready},8,true});
    assert(cpuKernelWord(address)==99 && !waits);
    cur->writebacks[0].hostReady=false;
    assert(cpuKernelWord(address)==77 && waits==1);
    ring[0].live=true; ring[0].submitSerial=3;
    ring[0].counters.push_back({shadow,7,2000,{}});
    OcclusionCounter query; query.reset(); auto part=query.segment();
    cur->counters.push_back({shadow,0,3000,query.snapshot()});
    assert(cpuKernelWord(address)==77 && waits==2); // cannot use older known 7
    part->value=9;part->ready=true;
    cur->counters.push_back({shadow,4,4000,query.snapshot()});
    assert(cpuKernelWord(address)==13 && waits==2);
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-counter-') as tmp:
            cpp = pathlib.Path(tmp) / 'counter.cpp'
            exe = pathlib.Path(tmp) / 'counter-test'
            cpp.write_text(code)
            subprocess.run([compiler, '-std=c++17', '-O2', '-I', str(ROOT / 'entry/src/main/cpp'),
                            str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_overlapping_writes(self):
        compiler = shutil.which('clang++') or shutil.which('c++')
        if not compiler:
            self.skipTest('requires a C++17 compiler')
        source = (ROOT / 'entry/src/main/cpp/gfx/dm_vk_replay.cpp').read_text()
        start = source.index('std::map<std::pair<uint64_t, uint64_t>, Slice> copies;')
        end = source.index('Slice zeroSlice', start)
        test = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <tuple>
#include "gfx/nvn/index_range.h"
struct Slice {};
''' + source[start:end] + r'''
int main() {
    uint8_t unaligned[32]{};
    uint16_t indices16[]{3, 65535, 18, 2};
    memcpy(unaligned + 1, indices16, sizeof indices16);
    assert(nvnIndexLimit(unaligned + 1, 4, 1) == 19);
    uint32_t indices32[]{5, UINT32_MAX, 100000, 2};
    memcpy(unaligned + 1, indices32, sizeof indices32);
    assert(nvnIndexLimit(unaligned + 1, 4, 2) == 100001);
    uint8_t indices8[]{5, 255, 2};
    assert(nvnIndexLimit(indices8, 3, 0) == 256);
    assert(nvnIndexLimit(indices8, 0, 0) == 0);
    copies[{100, 20}] = {};
    copies[{120, 20}] = {};
    copies[{1000, 20}] = {};
    indexRanges[{100, 10, 1}] = 9; // 20 bytes
    indexRanges[{1000, 10, 2}] = 9; // 40 bytes
    invalidateCopies(120, 1);
    assert(copies.count({100, 20}) && !copies.count({120, 20}));
    assert(indexRanges.count({100, 10, 1})); // touching boundary is disjoint
    invalidateCopies(110, 1);
    assert(!copies.count({100, 20}) && !indexRanges.count({100, 10, 1}));
    assert(copies.count({1000, 20}) && indexRanges.count({1000, 10, 2}));
    invalidateCopies(1025, 1); // overlaps 32-bit indices, not the shorter slice
    assert(copies.count({1000, 20}) && indexRanges.empty());
    invalidateCopies(1000, 0);
    assert(copies.size() == 1);
    copies[{1, 64ull << 20}] = {};
    invalidateCopies(64ull << 20, 1); // last byte of maximum-sized slice
    assert(!copies.count({1, 64ull << 20}));
    invalidateCopies(UINT64_MAX - 1, 4); // malformed range clears conservatively
    assert(copies.empty() && indexRanges.empty());
}
'''
        with tempfile.TemporaryDirectory(prefix='d2r-cache-') as tmp:
            cpp = pathlib.Path(tmp) / 'cache.cpp'
            exe = pathlib.Path(tmp) / 'cache-test'
            cpp.write_text(test)
            subprocess.run([compiler, '-std=c++17', '-O2',
                            '-I', str(ROOT / 'entry/src/main/cpp'),
                            str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    unittest.main()
