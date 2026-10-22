#pragma once
#include "nvn_obj.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
extern "C" {
#include "gfx/tex/nvn_deswizzle.h"
}

// Resolve NVN's mip/array view and byte pitches before touching host or GPU
// memory. Array layers do not shrink with the mip level; 3D depth does.
struct TextureCopyPlan {
    unsigned level{}, layer{}, count{}, width{}, height{}, mipWidth{}, mipHeight{}, mipDepth{};
    int blockWidth{}, blockHeight{}, blockBytes{};
    bool volume{};
    size_t row{}, image{}, extent{}, tightRow{}, rows{};
};
inline uint64_t textureLayerCount(const nvn_texdesc &d) {
    return d.target == 8 ? 6 : d.target == 9 ? uint64_t(std::max(1u, d.depth)) * 6 :
           d.target == 3 ? std::max(1u, d.height) : d.target == 4 || d.target == 6 ? std::max(1u, d.depth) : 1;
}
inline bool textureCopyPlan(const nvn_texdesc &d, const view_t *v, const NVNcopyRegion &r,
                            int64_t rowStride, int64_t imageStride, TextureCopyPlan &out) {
    out = {};
    if (d.samples > 1 || r.width <= 0 || r.height <= 0 || r.depth < 0 || r.xoffset < 0 || r.yoffset < 0 ||
        r.zoffset < 0 || rowStride < 0 || imageStride < 0 ||
        (v && ((v->set_levels && v->base_level < 0) || (v->set_layers && v->min_layer < 0)))) return false;
    out.level = v && v->set_levels ? unsigned(v->base_level) : 0;
    if (out.level >= std::max(1u, d.levels) || out.level >= 32) return false;
    out.volume = d.target == 2;
    out.mipWidth = std::max(1u, d.width >> out.level);
    out.mipHeight = d.target == 0 || d.target == 3 ? 1 : std::max(1u, d.height >> out.level);
    out.mipDepth = out.volume ? std::max(1u, d.depth >> out.level) : 1;
    out.width = unsigned(r.width); out.height = unsigned(r.height); out.count = unsigned(std::max(1, r.depth));
    if (unsigned(r.xoffset) > out.mipWidth || out.width > out.mipWidth - unsigned(r.xoffset) ||
        unsigned(r.yoffset) > out.mipHeight || out.height > out.mipHeight - unsigned(r.yoffset)) return false;
    uint64_t layers = textureLayerCount(d);
    unsigned first = !out.volume && v && v->set_layers ? unsigned(v->min_layer) : 0;
    uint64_t available = out.volume ? out.mipDepth : layers;
    if (first >= available) return false;
    available -= first;
    if (!out.volume && v && v->set_layers && v->nlayers > 0)
        available = std::min(available, uint64_t(v->nlayers));
    if (unsigned(r.zoffset) > available || out.count > available - unsigned(r.zoffset)) return false;
    out.layer = first + unsigned(r.zoffset);
    out.blockBytes = nvn_fmt_bpb(d.format, &out.blockWidth, &out.blockHeight);
    if (!out.blockBytes || r.xoffset % out.blockWidth || r.yoffset % out.blockHeight) return false;
    size_t blocks = (size_t(out.width) + out.blockWidth - 1) / out.blockWidth;
    out.rows = (size_t(out.height) + out.blockHeight - 1) / out.blockHeight;
    if (blocks > SIZE_MAX / size_t(out.blockBytes)) return false;
    out.tightRow = blocks * out.blockBytes;
    out.row = rowStride ? size_t(rowStride) : out.tightRow;
    if (out.row < out.tightRow || out.row > SIZE_MAX / out.rows) return false;
    out.image = imageStride ? size_t(imageStride) : out.row * out.rows;
    if (out.image < out.row * out.rows || out.image > SIZE_MAX / out.count) return false;
    out.extent = out.image * out.count;
    return true;
}

inline bool readPackagedTexture(const tex_t &t, const TextureCopyPlan &plan, const NVNcopyRegion &r,
                                 uint8_t *dst) {
    if (!t.mem || !t.has_layout || !dst || (t.d.flags & 0x10)) return false;
    size_t pitch = (size_t(plan.mipWidth) + plan.blockWidth - 1) / plan.blockWidth * plan.blockBytes;
    size_t rows = (size_t(plan.mipHeight) + plan.blockHeight - 1) / plan.blockHeight;
    if (!pitch || pitch > SIZE_MAX / rows || pitch * rows > (256u << 20) / plan.mipDepth) return false;
    std::vector<uint8_t> linear(pitch * rows * plan.mipDepth);
    for (unsigned z = 0; z < plan.count; z++) {
        if (!plan.volume || z == 0)
            if (nvn_deswizzle(&t.d, plan.level, plan.volume ? 0 : plan.layer + z,
                              t.mem, linear.data(), pitch)) return false;
        size_t offset = plan.volume ? size_t(plan.layer + z) * pitch * rows : 0;
        offset += size_t(r.yoffset / plan.blockHeight) * pitch +
                  size_t(r.xoffset / plan.blockWidth) * plan.blockBytes;
        for (size_t y = 0; y < plan.rows; y++)
            memcpy(dst + z * plan.image + y * plan.row, linear.data() + offset + y * pitch, plan.tightRow);
    }
    return true;
}
