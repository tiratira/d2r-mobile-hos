# dmvk_harness — PC 端 Vulkan 回放校验台

设备侧: 设置环境变量 `DM_DUMP_FRAME=1` 启动 app,会在
`/data/storage/el2/base/haps/entry/files/frames/` 写出 `f000.dump`(命令流)与 `f000.bin`(顶点原始字节)。

PC 侧:

```
cmake -S . -B build -DVULKAN_SDK=<sdk>
cmake --build build
./build/dmvk_harness <dump 目录> <着色器目录>
```

着色器目录形如 `spv_scrub/<key>.spv` + `spv/<key>.res.json`(与设备 `files/shaders/` 相同命名)。

工具会:
1. 解析 dump 的每条 PROG/DRAW 与逐命令顶点布局(attrs/streams);
2. 创建 headless Vulkan 实例(启用 `VK_LAYER_KHRONOS_validation`);
3. 用 dump 里的布局建 `VkVertexInput*` + `VkPipeline`,逐条打印 `VkResult` 与 VUID 错误;
4. 输出汇总:成功/失败条数、失败样例 → 直接定位 `gpipe_err=-1` 的原因。
