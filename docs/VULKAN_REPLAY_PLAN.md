# NVN → Vulkan 回放移植计划（剩余工作）

真机现在已经**稳定运行在游戏自己的帧循环里**（见 `docs/PORTING.md` 第 12 节）：
`nvnQueueSubmitCommands / nvnWindowPresent` 每帧发生，Vulkan 呈现 ~100 fps，
屏幕上是游戏 `ClearColor` 的真实清屏色。

要让**游戏画面**真正出现，剩下的工作是把上游的 NVN 记录/回放层移植到 Vulkan。
这是一项独立的大工程，下面给出可直接执行的阶段划分、依赖和验收点。

## 0. 现状与差距

| 层 | 上游 (Metal) | 本仓库 (HOS) 现状 |
|---|---|---|
| NVN 接口层 | `gfx/nvn/hle_nvn.c` 136 KB，203 个入口真实实现 | `gfx/nvn/dm_nvn.c` 最小对象模型，命令不落盘 |
| 对象布局 | `gfx/nvn/nvn_obj.h`（含 FITS 断言） | 仅少量 overlay（已按 `nvn.h` 校验尺寸） |
| 命令流 | 录制进游戏给的命令内存 | 只计数，不录制 |
| 回放层 | `gfx/nvn/nvn_mtl.m` 675 KB / 6534 行（Metal 4 + Metal 3） | 无（`gfx/dm_vk.cpp` 只画清屏色） |
| 纹理 | Tegra block-linear deswizzle + BC 解码 + 格式转换（`gfx/tex/*`） | 无 |
| 着色器 | 离线 Maxwell→SPIR-V（Ryujinx）→MSL→metallib | 无 |
| 同步/present | `nvn_pace.h` + Metal 呈现 | Vulkan 清屏循环已通 |

## 1. 前置条件（S0）

1. **着色器翻译工具链**
   - 仓内已有 `dm/third_party/ryujinx` 源码 + `dm/gfx/tools/dmsh_xlate`（.NET 10）。
   - 本机只装了 .NET **runtime**，需要 SDK 10：
     `winget install Microsoft.DotNet.SDK.10` 或 `dotnet-install.ps1` 装到用户目录。
   - 用 `dotnet publish dm/gfx/tools/dmsh_xlate -c Release -o build/gfx/dmsh_xlate` 构建，
     跑 `dmsh_xlate --source-sha` 自检。
   - 与上游不同的是：**Vulkan 路线直接用第一步的 SPIR-V**，不需要 spirv-cross→MSL→metallib，
     因此也不需要 macOS/Xcode。反射（binding 位置）可由 SPIR-V 解析或 `spirv-cross --reflect` 生成。

2. **游戏着色器包（162 个 .bin）**
   - 本仓库不含游戏内容；真机 RomFS 里有（`romfs/data/hd/shaders/*.bin`）。
   - 需要先做一个**宿主侧清单能力**：在 app 内扫描 romfs 该目录并把清单写到
     `/data/storage/el2/base/haps/entry/files/romfs_shaders.txt`，
     再用 `hdc file recv -b com.d2rmobile.hos <远端> <本地>` 取回；随后逐个拉取 shader pack。
   - 提取后按上游 `setup.sh` 的 `packs()` 逻辑（`dm/tools/dmx.py get`）落盘到
     `build/cache/dm/dmsh/`，再跑 `dmsh_build.py` 的 SPIR-V 阶段。

3. **Vulkan 能力确认（真机）**
   - Vulkan 版本、`VK_EXT_robustness2`、`bufferDeviceAddress`、纹理压缩格式支持；
   - 计划用**零拷贝统一内存**：内存池 = host-visible/device-local buffer +
     `vkGetBufferDeviceAddress`，使 NVN 的 GPU 地址 == CPU 地址（与上游 Metal 思路一致）。

## 2. 分层移植（S1–S5）

### S1 记录层：把 `hle_nvn.c` 的行为搬进 HOS 宿主
- 以 `nvn_obj.h` 为对象布局蓝本，逐个入口实现：device / queue / commandBuffer /
  memoryPool / buffer / texture / sampler / program / window / sync / event。
- 命令按 `nvn_obj.h` 的 record 格式写进游戏给的命令内存
  （`nvnCommandBufferAddCommandMemory` 已能拿到 pool+offset+size）。
- 保留上游的 `FITS()` 静态断言，防止 overlay 越界（此前的 `nvn_queueb_t` 68>64 就是这类 bug）。
- 验收：一帧命令内存字节流与上游（同一 API 序列）逐字节一致（可用 `t_nvn.c` 的思路做单测）。

### S2 资源层：内存池与纹理
- 内存池：`VkBuffer` + host-visible 内存 + `bufferDeviceAddress`，建立 GPU 地址映射表；
- 纹理：Tegra block-linear → linear（`gfx/tex/nvn_deswizzle.c` 可移植），
  BC1/BC3/BC5/BC7 解码（`nvn_bcdec.c`），格式→`VkFormat` 映射（`nvn_fmt` 对应表）；
- 纹理视图/采样器状态：按 NVN texture/sampler 对象建立 `VkImageView`/`VkSampler` 缓存。
- 验收：纹理上传单测（与上游 MSL 路线的解码结果做像素级比对）。

### S3 管线与描述符
- 以 NVN 状态哈希（blend/depth/stencil/raster/multisample/vertex attrib）为键缓存 `VkPipeline`；
- 从 SPIR-V 反射生成 `VkDescriptorSetLayout`：uniform buffer / separate texture / sampler /
  storage buffer / image 的 binding 约定按上游 `maxwell_refs.py` 文件头；
- 顶点流/属性、index buffer 直接映射到 `vkCmdBindVertexBuffers/vkCmdBindIndexBuffer`。
- 验收：首帧用到的 PSO 全部创建成功（用 `pso_manifest.tsv` 做预热清单）。

### S4 命令回放
- 渲染通道：color/depth attachments、clear/load/store、resolve；
- copy/blit（buffer↔texture、texture↔texture）、barrier（`vkCmdPipelineBarrier`）；
- draw / drawIndexed / instanced / indirect、compute dispatch / dispatchIndirect；
- 同步：NVN sync/event/fence → `VkFence`/`VkSemaphore`/timeline semaphore；
- 验收：单帧回放无 Vulkan validation error（开发期开 validation layer）。

### S5 呈现与帧调度
- `nvnWindowAcquireTexture/PresentTexture` → swapchain acquire/present；
- `nvnQueueFenceSync/nvnSyncWait` → 真实 GPU 完成；
- 帧节奏沿用现有 Vulkan 循环，逐步接入 `nvn_pace` 的等 vsync 逻辑；
- 验收：真机连续运行 ≥10 分钟，present 稳定、无泄漏、画面与上游 Metal 版截图接近。

## 3. 里程碑

| 里程碑 | 内容 | 验收 |
|---|---|---|
| M0 | 着色器 SPIR-V 全量生成（162 packs） | `_b_build.json` 全 ok |
| M1 | 记录层 + 对象模型完成 | 命令流单测通过 |
| M2 | 资源层（pool/texture）完成 | 纹理像素比对通过 |
| M3 | 管线/描述符完成 | 首帧 PSO 全创建 |
| M4 | 回放 + present 接通 | 真机出现游戏首帧画面 |
| M5 | 稳定/性能 | 10 分钟稳定，≥30 fps |

## 4. 风险

1. **工作量**：上游回放层 6534 行 Metal 代码，逐行移植到 Vulkan 是数周级任务，不是一次提交能完成的；
2. **真机 Vulkan 能力**：bufferDeviceAddress / 大内存 host-visible 分配 / 压缩格式支持需要先探测；
3. **着色器反射**：SPIR-V 的 binding 约定必须与上游 MSL 一致，否则绑定错位；
4. **纹理格式种类多**：Tegra swizzle + BC 解码 + sRGB/深度格式容易出细节错误；
5. **法律边界**：着色器包与游戏数据只从用户自己的游戏文件离线生成，不入库、不随包分发。

## 5. 建议的下一步（可直接开始）

1. 安装 .NET SDK 10，构建 `dmsh_xlate` 并跑通 `--source-sha`；
2. 给 app 增加 romfs 清单 + shader pack 拉取，把 162 个 pack 取到构建缓存；
3. 生成 SPIR-V 全量集合（M0），同时写一个最小的 Vulkan 端到端 demo
   （一个三角形 + 内置 SPIR-V）验证 S2–S4 的骨架；
4. 再按 S1→S5 顺序推进。
