// dmvk_harness: 离线回放设备导出的 f000.dump,在 PC 上用 VVL 校验管线创建。
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

struct Op {
    uint32_t kind; char k0[17], k1[17];
    uint32_t a, b, nvb; uint64_t vba[4], vbs[4];
    int32_t nattr, nstream; uint32_t attrs[16], streams[32];
};

static std::vector<uint8_t> read_file(const std::string &p)
{
    std::vector<uint8_t> v;
    FILE *f = fopen(p.c_str(), "rb");
    if (!f) return v;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    v.resize((size_t)n);
    if (fread(v.data(), 1, (size_t)n, f) != (size_t)n) v.clear();
    fclose(f);
    return v;
}

static VkFormat nvn_fmt_to_vk(uint32_t f, int comps)
{
    switch (f) {
        case 0x25: return VK_FORMAT_R8G8B8A8_UNORM;
        case 0x26: return VK_FORMAT_R8G8B8A8_SNORM;
        case 0x27: return VK_FORMAT_R8G8B8A8_UINT;
        case 0x28: return VK_FORMAT_R8G8B8A8_SINT;
        case 0x2E: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case 0x5F: return VK_FORMAT_R16G16B16_USCALED;
        default: break;
    }
    switch (comps) {
        case 1: return VK_FORMAT_R32_SFLOAT;
        case 2: return VK_FORMAT_R32G32_SFLOAT;
        case 3: return VK_FORMAT_R32G32B32_SFLOAT;
        default: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

static VkShaderModule load_module(VkDevice dev, const std::string &path)
{
    auto code = read_file(path);
    if (code.size() < 20) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule m = VK_NULL_HANDLE;
    if (vkCreateShaderModule(dev, &ci, nullptr, &m) != VK_SUCCESS) return VK_NULL_HANDLE;
    return m;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s <dumpdir> <shaderdir>\n", argv[0]); return 2; }
    std::string dumpdir = argv[1], shdir = argv[2];
    auto raw = read_file(dumpdir + "/f000.dump");
    if (raw.size() < 8) { fprintf(stderr, "no f000.dump\n"); return 2; }
    uint32_t magic, count;
    memcpy(&magic, raw.data(), 4); memcpy(&count, raw.data() + 4, 4);
    if (magic != 0x464D5244u) { fprintf(stderr, "bad magic\n"); return 2; }
    std::vector<Op> ops(count);
    size_t off = 8;
    for (uint32_t i = 0; i < count; i++) {
        Op &o = ops[i];
        o.nvb = 0; o.nattr = o.nstream = 0;
        memcpy(&o.kind, raw.data() + off, 4); off += 4;
        memcpy(o.k0, raw.data() + off, 17); off += 17;
        memcpy(o.k1, raw.data() + off, 17); off += 17;
        memcpy(&o.a, raw.data() + off, 4); off += 4;
        memcpy(&o.b, raw.data() + off, 4); off += 4;
        memcpy(&o.nvb, raw.data() + off, 4); off += 4;
        if (o.nvb > 4) { fprintf(stderr, "bad nvb\n"); return 2; }
        memcpy(o.vba, raw.data() + off, 8 * o.nvb); off += 8 * o.nvb;
        memcpy(o.vbs, raw.data() + off, 8 * o.nvb); off += 8 * o.nvb;
        memcpy(&o.nattr, raw.data() + off, 4); off += 4;
        if (o.nattr > 16) { fprintf(stderr, "bad nattr\n"); return 2; }
        if (o.nattr > 0) { memcpy(o.attrs, raw.data() + off, 4 * o.nattr); off += 4 * o.nattr; }
        memcpy(&o.nstream, raw.data() + off, 4); off += 4;
        if (o.nstream > 32) { fprintf(stderr, "bad nstream\n"); return 2; }
        if (o.nstream > 0) { memcpy(o.streams, raw.data() + off, 4 * o.nstream); off += 4 * o.nstream; }
    }
    printf("dump: %u ops (parse ok)\n", count);

    const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ai.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &ai;
    ici.enabledLayerCount = 1; ici.ppEnabledLayerNames = layers;
    const char *exts[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    ici.enabledExtensionCount = 1; ici.ppEnabledExtensionNames = exts;
    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&ici, nullptr, &inst) != VK_SUCCESS) {
        ici.enabledLayerCount = 0; ici.enabledExtensionCount = 0;
        if (vkCreateInstance(&ici, nullptr, &inst) != VK_SUCCESS) { fprintf(stderr, "no Vulkan instance\n"); return 2; }
    }
    uint32_t ndev = 0; vkEnumeratePhysicalDevices(inst, &ndev, nullptr);
    if (ndev == 0) { fprintf(stderr, "no Vulkan device\n"); return 2; }
    std::vector<VkPhysicalDevice> pds(ndev); vkEnumeratePhysicalDevices(inst, &ndev, pds.data());
    VkPhysicalDevice pd = pds[0];
    uint32_t nqf = 0; vkGetPhysicalDeviceQueueFamilyProperties(pd, &nqf, nullptr);
    std::vector<VkQueueFamilyProperties> qf(nqf); vkGetPhysicalDeviceQueueFamilyProperties(pd, &nqf, qf.data());
    uint32_t qi = 0; for (uint32_t i = 0; i < nqf; i++) if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { qi = i; break; }
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = qi; qci.queueCount = 1; qci.pQueuePriorities = &prio;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
    VkPhysicalDeviceFeatures feats{};
    dci.pEnabledFeatures = &feats;
    VkDevice dev = VK_NULL_HANDLE;
    if (vkCreateDevice(pd, &dci, nullptr, &dev) != VK_SUCCESS) { fprintf(stderr, "no device\n"); return 2; }

    VkAttachmentDescription att{};
    att.format = VK_FORMAT_R8G8B8A8_UNORM; att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference ar{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{}; sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1; sub.pColorAttachments = &ar;
    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 1; rpci.pAttachments = &att;
    rpci.subpassCount = 1; rpci.pSubpasses = &sub;
    VkRenderPass rp = VK_NULL_HANDLE;
    vkCreateRenderPass(dev, &rpci, nullptr, &rp);

    int tried = 0, ok = 0, fail = 0;
    std::string last_pair;
    for (uint32_t i = 0; i < count; i++) {
        Op &o = ops[i];
        if (o.kind != 0) continue;   // 只看 PROG(管线创建)
        std::string vs = shdir + "/" + o.k0 + ".spv";
        std::string ps = shdir + "/" + o.k1 + ".spv";
        if (read_file(vs).empty() || read_file(ps).empty()) continue;
        tried++;
        VkShaderModule vm = load_module(dev, vs), pm = load_module(dev, ps);
        if (vm == VK_NULL_HANDLE || pm == VK_NULL_HANDLE) { fail++; continue; }
        VkPipelineShaderStageCreateInfo st[2]{};
        st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vm; st[0].pName = "main";
        st[1] = st[0]; st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = pm;
        VkVertexInputBindingDescription vibs[8]{}; uint32_t nvb = 0;
        VkVertexInputAttributeDescription vias[16]{}; uint32_t nva = 0;
        for (int k = 0; k < o.nattr && k < 16; k++) {
            uint32_t stream = (o.attrs[k] >> 27) & 0x1F;
            uint32_t offb = (o.attrs[k] >> 8) & 0x7FFFF;
            if (stream >= 8) continue;
            uint32_t stride = (o.nstream > (int)(stream * 2 + 1)) ? o.streams[stream * 2] : 32;
            bool have = false;
            for (uint32_t b = 0; b < nvb; b++) if (vibs[b].binding == stream) have = true;
            if (!have && nvb < 8) { vibs[nvb].binding = stream; vibs[nvb].stride = stride; vibs[nvb].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; nvb++; }
            vias[nva].location = (uint32_t)k; vias[nva].binding = stream; vias[nva].offset = offb;
            vias[nva].format = nvn_fmt_to_vk(o.attrs[k] & 0xFF, 4); nva++;
        }
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        if (nva > 0) { vi.vertexBindingDescriptionCount = nvb; vi.pVertexBindingDescriptions = vibs;
                       vi.vertexAttributeDescriptionCount = nva; vi.pVertexAttributeDescriptions = vias; }
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount = 1; vp.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState cba{}; cba.colorWriteMask = 0xF;
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount = 1; cb.pAttachments = &cba;
        VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        ds.dynamicStateCount = 2; ds.pDynamicStates = dyn;
        VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        VkPipelineLayout pl = VK_NULL_HANDLE;
        vkCreatePipelineLayout(dev, &plci, nullptr, &pl);
        VkGraphicsPipelineCreateInfo gpci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        gpci.stageCount = 2; gpci.pStages = st;
        gpci.pVertexInputState = &vi; gpci.pInputAssemblyState = &ia;
        gpci.pViewportState = &vp; gpci.pRasterizationState = &rs;
        gpci.pMultisampleState = &ms; gpci.pColorBlendState = &cb;
        gpci.pDynamicState = &ds; gpci.layout = pl; gpci.renderPass = rp;
        VkPipeline pipe = VK_NULL_HANDLE;
        VkResult r = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, nullptr, &pipe);
        if (pipe) vkDestroyPipeline(dev, pipe, nullptr);
        vkDestroyPipelineLayout(dev, pl, nullptr);
        vkDestroyShaderModule(dev, vm, nullptr);
        vkDestroyShaderModule(dev, pm, nullptr);
        if (r == VK_SUCCESS) { ok++; if (tried <= 3) printf("  ok   %s %s attrs=%d\n", o.k0, o.k1, o.nattr); }
        else { fail++; if (tried <= 6) printf("  FAIL %s %s attrs=%d streams=%d r=%d layout:", o.k0, o.k1, o.nattr, o.nstream, (int)r);
               for (int k = 0; k < o.nattr; k++) printf(" [fmt=0x%02x off=%u st=%u str=%u]", o.attrs[k] & 0xFF, (o.attrs[k] >> 8) & 0x7FFFF, (o.attrs[k] >> 27) & 0x1F,
                   (o.nstream > (int)(((o.attrs[k] >> 27) & 0x1F) * 2 + 1)) ? o.streams[((o.attrs[k] >> 27) & 0x1F) * 2] : 0);
               printf("\n"); }
    }
    printf("pipelines: tried=%d ok=%d fail=%d\n", tried, ok, fail);
    vkDestroyRenderPass(dev, rp, nullptr);
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
    return fail ? 1 : 0;
}
