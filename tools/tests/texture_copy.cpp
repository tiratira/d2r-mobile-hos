// Host regression fixture. test_texture_copy.py inserts the real replay code.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "gfx/nvn/texture_copy.h"
extern "C" {
#include "gfx/tex/nvn_bcdec.h"
}
#include <cassert>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

struct Slice { int b{}; size_t off{},size{}; uint8_t *mapped{}; };
struct Texture {
    VkImage image{}; VkFormat format{};
    uint32_t w{},h{},depth{1},layers{1},levels{1},texelBytes{4};
    int target{}; uint64_t serial{};
    VkImageAspectFlags aspect{VK_IMAGE_ASPECT_COLOR_BIT};
    std::map<std::string,VkImageView> views;
};
struct Writeback { uint64_t addr; Slice slice; size_t size; };
struct Capture { std::string name; Slice slice; };
struct Slot { std::vector<Writeback> writebacks; std::vector<VkFramebuffer> framebuffers;
              std::vector<Capture> captures; } slot, *cur=&slot;
struct Pass { VkRenderPass pass{}; };
std::map<std::string,Pass> passes;
std::map<std::pair<uint64_t,size_t>,Slice> copies;
std::map<int,std::vector<uint8_t>> buffers;
std::map<uint64_t,Texture> textures;
std::vector<VkImageCopy> imageCopies;
std::vector<VkImageBlit> imageBlits;
std::vector<VkImageViewCreateInfo> imageViews;
std::vector<VkClearAttachment> clears;
std::vector<VkClearRect> clearRects;
std::vector<VkFormat> passFormats;
VkBufferImageCopy upload{},download{};
std::vector<uint8_t> uploadBytes;
VkFilter blitFilter{};
VkDevice dev{}; VkCommandBuffer cmd{};
std::recursive_mutex mu;
bool ready=true, deviceLost=false;
struct { int64_t rowStride{},imageStride{}; } S;
int drained{},gpuWrites{},invalidations{},begins{},finishes{},reclaims{};
uint64_t nextHandle=10;
template<class T> T handle() { return reinterpret_cast<T>(uintptr_t(nextHandle++)); }
template<class... A> void note(A...) {}
bool check(VkResult result,const char*) { assert(result==VK_SUCCESS); return true; }
void endpass() {}
void imageBarrier(Texture&,VkImageLayout,VkImageLayout) {}
Texture *texture(const tex_t *t) { return t ? &textures.at(t->serial) : nullptr; }
Slice bytes(const void *src,size_t n) {
    int id=int(buffers.size())+1; auto &b=buffers[id]; b.resize(n);
    if(src) memcpy(b.data(),src,n);
    return {id,0,n,b.data()};
}
Slice guest(uint64_t address,size_t n,bool) { return bytes((void*)(uintptr_t)address,n); }
void drainIfOverlap(uint64_t,size_t) { drained++; }
void recordGpuWrite(uint64_t,size_t) { gpuWrites++; }
void invalidateCopies(uint64_t,size_t) { invalidations++; copies.clear(); }
void begin() { begins++; }
void finish() { finishes++; }
void reclaim(Slot &s) {
    reclaims++;
    for(auto &w:s.writebacks) memcpy((void*)(uintptr_t)w.addr,w.slice.mapped,w.size);
    s.writebacks.clear();
}
void reclaimAll() { if(!slot.writebacks.empty()) reclaim(slot); }
void vkCmdCopyBufferToImage(VkCommandBuffer,int b,VkImage,VkImageLayout,uint32_t n,const VkBufferImageCopy *p) {
    assert(n==1); upload=*p; uploadBytes=buffers.at(b);
}
void vkCmdCopyImageToBuffer(VkCommandBuffer,VkImage,VkImageLayout,int b,uint32_t n,const VkBufferImageCopy *p) {
    assert(n==1); download=*p; auto &out=buffers.at(b);
    // This fixture's GPU image contains a distinct value for each layer/mip.
    unsigned pitch=(p->bufferRowLength ? p->bufferRowLength : p->imageExtent.width)*4;
    unsigned image=pitch*(p->bufferImageHeight ? p->bufferImageHeight : p->imageExtent.height);
    unsigned count=std::max(p->imageSubresource.layerCount,p->imageExtent.depth);
    for(unsigned z=0;z<count;z++) for(unsigned y=0;y<p->imageExtent.height;y++) {
        size_t off=p->bufferOffset+z*image+y*pitch;
        assert(off+p->imageExtent.width*4<=out.size());
        memset(out.data()+off,int(0x40+p->imageSubresource.mipLevel+p->imageSubresource.baseArrayLayer+z),
               p->imageExtent.width*4);
    }
}
void vkCmdCopyImage(VkCommandBuffer,VkImage,VkImageLayout,VkImage,VkImageLayout,uint32_t n,const VkImageCopy *p) {
    imageCopies.assign(p,p+n);
}
void vkCmdBlitImage(VkCommandBuffer,VkImage,VkImageLayout,VkImage,VkImageLayout,uint32_t n,
                    const VkImageBlit *p,VkFilter f) { imageBlits.assign(p,p+n); blitFilter=f; }
template<class... A> void vkCmdPipelineBarrier(A...) {}
VkResult vkCreateImageView(VkDevice,const VkImageViewCreateInfo *p,void*,VkImageView *out) {
    imageViews.push_back(*p); *out=handle<VkImageView>(); return VK_SUCCESS;
}
VkResult vkCreateRenderPass(VkDevice,const VkRenderPassCreateInfo *p,void*,VkRenderPass *out) {
    passFormats.push_back(p->pAttachments[0].format); *out=handle<VkRenderPass>(); return VK_SUCCESS;
}
VkResult vkCreateFramebuffer(VkDevice,const VkFramebufferCreateInfo*,void*,VkFramebuffer *out) {
    *out=handle<VkFramebuffer>(); return VK_SUCCESS;
}
void vkCmdBeginRenderPass(VkCommandBuffer,const VkRenderPassBeginInfo*,VkSubpassContents) {}
void vkCmdEndRenderPass(VkCommandBuffer) {}
void vkCmdClearAttachments(VkCommandBuffer,uint32_t n,const VkClearAttachment *a,uint32_t m,const VkClearRect *r) {
    assert(n==1 && m==1); clears.push_back(*a); clearRects.push_back(*r);
}

// ACTUAL_REPLAY_CODE

tex_t asset(int target,int formatCode,unsigned w,unsigned h,unsigned depth,unsigned levels,uint64_t serial) {
    tex_t t{}; t.d.target=target; t.d.format=formatCode; t.d.width=w; t.d.height=h;
    t.d.depth=depth; t.d.levels=levels; t.serial=serial;
    Texture gpu; gpu.image=handle<VkImage>(); gpu.format=format(formatCode); gpu.serial=serial;
    gpu.w=w; gpu.h=h; gpu.depth=target==2 ? depth : 1;
    gpu.layers=uint32_t(textureLayerCount(t.d)); gpu.levels=levels; gpu.target=target;
    int bw,bh; gpu.texelBytes=nvn_fmt_bpb(formatCode,&bw,&bh);
    if(bw!=1 || bh!=1) { uint32_t pf; int bpp; assert(nvn_bc_unpacked(formatCode,&pf,&bpp));
        gpu.texelBytes=bpp; gpu.format=VK_FORMAT_R8G8B8A8_UNORM; }
    textures[serial]=gpu; return t;
}
void resetCalls() {
    S={}; imageCopies.clear(); imageBlits.clear(); uploadBytes.clear(); upload={}; download={};
    slot.writebacks.clear(); copies.clear(); drained=gpuWrites=invalidations=begins=finishes=reclaims=0;
}
void test_upload_and_readback() {
    auto t=asset(4,0x25,16,16,8,5,1);
    std::vector<uint8_t> source(128,0x9d);
    p_copy_b2t c{}; c.tex=&t; c.src=uint64_t(uintptr_t(source.data())); c.has_view=1;
    c.view.set_levels=c.view.set_layers=1; c.view.base_level=2; c.view.min_layer=3; c.view.nlayers=4;
    c.r={1,0,1,2,2,2};
    resetCalls(); S.rowStride=16; S.imageStride=64; copyB2T(c);
    assert(upload.imageSubresource.mipLevel==2 && upload.imageSubresource.baseArrayLayer==4);
    assert(upload.imageSubresource.layerCount==2 && upload.imageOffset.z==0 && upload.imageExtent.depth==1);
    assert(upload.bufferRowLength==4 && upload.bufferImageHeight==4 && uploadBytes==source);
    std::vector<uint8_t> destination(128,0xa5); c.src=uint64_t(uintptr_t(destination.data()));
    resetCalls(); assert(copyT2B(c,16,64));
    assert(gpuWrites==1 && invalidations==1 && slot.writebacks.size()==1);
    assert(destination[0]==0xa5); // host publication waits for completion
    auto staged=slot.writebacks[0].slice;
    assert(staged.mapped[0]==0x46 && staged.mapped[64]==0x47);
    assert(staged.mapped[8]==0xa5 && staged.mapped[32]==0xa5 && staged.mapped[120]==0xa5);
    assert(copies.at({c.src,128}).b==staged.b);
    reclaim(slot); assert(destination[0]==0x46 && destination[64]==0x47 && destination[8]==0xa5);
    // CPU ReadTexels of a render target must actually get the GPU value.
    resetCalls(); std::fill(destination.begin(),destination.end(),0xa5);
    NVNcopyRegion r{1,0,1,2,2,2};
    mtl_read_texels(&t,2,3,&r,destination.data(),16,64);
    assert(begins==1 && finishes==1 && reclaims==1 && destination[0]==0x46 && destination[64]==0x47);
}
void test_bc_upload() {
    auto t=asset(4,0x41,16,16,6,5,2);
    uint8_t blocks[32]{}; blocks[1]=0xf8; blocks[16]=0xe0; blocks[17]=0x07; // red, green BC1 endpoints
    p_copy_b2t c{}; c.tex=&t; c.src=uint64_t(uintptr_t(blocks)); c.has_view=1;
    c.view.set_levels=c.view.set_layers=1; c.view.base_level=2; c.view.min_layer=2; c.view.nlayers=3;
    c.r={0,0,0,4,4,2}; resetCalls(); S.rowStride=8; S.imageStride=16; copyB2T(c);
    assert(drained==1 && uploadBytes.size()==128);
    for(int p=0;p<16;p++) {
        assert(uploadBytes[p*4]==255 && uploadBytes[p*4+1]==0 && uploadBytes[p*4+2]==0 && uploadBytes[p*4+3]==255);
        assert(uploadBytes[64+p*4]==0 && uploadBytes[65+p*4]==255 && uploadBytes[66+p*4]==0 && uploadBytes[67+p*4]==255);
    }
    assert(upload.imageSubresource.mipLevel==2 && upload.imageSubresource.baseArrayLayer==2);
    assert(upload.imageSubresource.layerCount==2 && upload.bufferRowLength==0 && upload.bufferImageHeight==0);
    // A final 1x1 mip still occupies one BC block in the source, but only one
    // RGBA texel in the Vulkan image. Do not upload the eight encoded bytes.
    resetCalls(); c.view.base_level=4; c.r={0,0,0,1,1,1}; copyB2T(c);
    assert((uploadBytes==std::vector<uint8_t>{255,0,0,255}));
}
void test_texture_copy_views_and_mirrors() {
    auto s=asset(4,0x0a,16,16,8,5,3), d=asset(4,0x0b,16,16,8,5,4);
    p_copy_t2t c{}; c.src=&s; c.dst=&d; c.has_sv=c.has_dv=1;
    c.sv.set_levels=c.sv.set_layers=c.dv.set_levels=c.dv.set_layers=1;
    c.sv.base_level=2; c.sv.min_layer=2; c.dv.base_level=1; c.dv.min_layer=5;
    c.sr={1,0,0,2,2,2}; c.dr={2,1,0,2,2,2}; resetCalls(); copyT2T(c);
    assert(imageCopies.size()==2 && imageBlits.empty()); // equal-size raw copy, no float/integer conversion
    for(unsigned z=0;z<2;z++) {
        auto &copy=imageCopies[z]; assert(copy.srcSubresource.mipLevel==2 && copy.dstSubresource.mipLevel==1);
        assert(copy.srcSubresource.baseArrayLayer==2+z && copy.dstSubresource.baseArrayLayer==5+z);
        assert(copy.srcOffset.z==0 && copy.dstOffset.z==0 && copy.extent.depth==1);
    }
    auto mirrorTarget=asset(4,0x0a,16,16,8,5,10); c.dst=&mirrorTarget;
    resetCalls(); c.flags=1|4|8|16; copyT2T(c); assert(imageBlits.size()==2 && imageCopies.empty());
    assert(blitFilter==VK_FILTER_LINEAR && imageBlits[0].srcSubresource.baseArrayLayer==3);
    assert(imageBlits[0].srcOffsets[0].x==3 && imageBlits[0].srcOffsets[1].x==1);
    assert(imageBlits[0].srcOffsets[0].y==2 && imageBlits[0].srcOffsets[1].y==0);
    auto volume=asset(2,0x25,8,8,8,3,5); std::vector<uint8_t> source(128);
    p_copy_b2t u{}; u.tex=&volume; u.src=uint64_t(uintptr_t(source.data())); u.has_view=1;
    u.view.set_levels=1; u.view.base_level=1; u.r={0,0,2,4,4,2}; resetCalls(); copyB2T(u);
    assert(upload.imageSubresource.baseArrayLayer==0 && upload.imageSubresource.layerCount==1);
    assert(upload.imageOffset.z==2 && upload.imageExtent.depth==2);
}
void test_packaged_mips_arrays_and_volume() {
    for(bool volume:{false,true}) {
        auto t=asset(volume?2:9,0x25,8,8,volume?8:3,3,volume?7:6);
        nvn_tex_layout(&t.d,&t.L); std::vector<uint8_t> storage(t.L.size,0); t.mem=storage.data(); t.has_layout=1;
        unsigned depth=volume?4:1;
        for(unsigned layer=0;layer<(volume?1:18);layer++) {
            std::vector<uint8_t> pixels(4*4*depth*4);
            for(unsigned z=0;z<depth;z++) for(unsigned y=0;y<4;y++) for(unsigned x=0;x<4;x++)
                for(unsigned c=0;c<4;c++) pixels[((z*4+y)*4+x)*4+c]=uint8_t(layer*7+z*30+y*5+x+c);
            assert(nvn_swizzle(&t.d,1,layer,pixels.data(),16,storage.data())==0);
        }
        view_t v{}; v.set_levels=v.set_layers=1; v.base_level=1; v.min_layer=volume?0:11;
        NVNcopyRegion r{1,1,1,2,2,2}; TextureCopyPlan plan;
        assert(textureCopyPlan(t.d,&v,r,12,36,plan));
        std::vector<uint8_t> out(plan.extent,0xa5); assert(readPackagedTexture(t,plan,r,out.data()));
        for(unsigned z=0;z<2;z++) for(unsigned y=0;y<2;y++) for(unsigned x=0;x<2;x++) for(unsigned c=0;c<4;c++)
            assert(out[z*36+y*12+x*4+c]==uint8_t((volume?0:(12+z)*7)+(volume?(1+z)*30:0)+(1+y)*5+1+x+c));
        assert(out[8]==0xa5 && out[24]==0xa5 && out.back()==0xa5);
        if(!volume) assert(textureLayerCount(t.d)==18);
        resetCalls(); std::fill(out.begin(),out.end(),0xa5);
        mtl_read_texels(&t,1,volume?0:11,&r,out.data(),12,36);
        assert(begins==0 && out[0]!=0xa5 && out.back()==0xa5);
    }
    auto t=asset(4,0x25,16,16,8,5,8); view_t v{}; TextureCopyPlan plan;
    NVNcopyRegion r{0,0,0,2,2,1};
    assert(!textureCopyPlan(t.d,nullptr,r,7,0,plan));
    assert(!textureCopyPlan(t.d,nullptr,r,8,15,plan));
    assert(!textureCopyPlan(t.d,nullptr,r,-1,0,plan));
    r.height=3; assert(!textureCopyPlan(t.d,nullptr,r,INT64_MAX,0,plan)); r.height=2;
    v.set_levels=1; v.base_level=32; assert(!textureCopyPlan(t.d,&v,r,0,0,plan));
    v.base_level=4; assert(!textureCopyPlan(t.d,&v,r,0,0,plan)); // mip is only 1x1
    v.base_level=0; v.set_layers=1; v.min_layer=7; r.depth=2;
    assert(!textureCopyPlan(t.d,&v,r,0,0,plan));
}
void test_integer_view_clear_preserves_bits() {
    auto t=asset(4,0x0a,16,16,8,5,9); resetCalls();
    p_clear_tex c{}; c.tex=&t; c.has_view=1; c.is_uint=1; c.mask=15;
    c.view.set_levels=c.view.set_layers=c.view.set_format=1;
    c.view.base_level=2; c.view.min_layer=3; c.view.format=0x0b;
    c.r={1,1,0,2,2,2}; c.c.u[0]=0xffffffff; clearTexture(c);
    assert(clears.size()==2 && passFormats.back()==VK_FORMAT_R32_UINT);
    assert(clears[0].clearValue.color.uint32[0]==0xffffffff);
    assert(imageViews[imageViews.size()-2].format==VK_FORMAT_R32_UINT);
    assert(imageViews[imageViews.size()-2].subresourceRange.baseMipLevel==2);
    assert(imageViews[imageViews.size()-2].subresourceRange.baseArrayLayer==3);
    assert(imageViews.back().subresourceRange.baseArrayLayer==4);
    assert(clearRects[0].rect.offset.x==1 && clearRects[0].rect.extent.width==2);
}
int main() {
    test_upload_and_readback(); test_bc_upload(); test_texture_copy_views_and_mirrors();
    test_packaged_mips_arrays_and_volume(); test_integer_view_clear_preserves_bits();
}
