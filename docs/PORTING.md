# d2r-mobile → HarmonyOS 移植指南(PORTING.md)

本文档说明如何把上游 [d2r-mobile](https://github.com/d2r-mobile/d2r-mobile)(在
iPhone/Mac 上原生运行 Switch 版《暗黑破坏神 II:重制版》)移植到 HarmonyOS。
先讲清上游的工作原理,再给出逐子系统的映射表、本仓库已完成的部分,以及
剩余工作的实施路线。

> 法律与内容边界:本仓库与上游一样,**不包含任何游戏程序、美术、文本、
> 数据表或着色器**。一切游戏内容由使用者从自己拥有的游戏文件离线生成;
> 上游要求自带 Nintendo Switch 版游戏文件(见上游 docs/GAME_FILES.md)。
> 本仓库仅为移植框架与接口骨架。

## 1. 上游如何工作(30 秒版)

Switch 版 D2R 是三个 NSO 模块(`main` / `sdk` / `subsdk0`)的 ARM64 原生
代码。iPhone 与 Switch 同为 ARM64,因此绝大多数指令可以**原样执行**,
需要处理的只有它们对 Switch 操作系统(Horizon)的依赖:

1. **离线重写**(`dm/tools/patch_nso.py`,Python,跑在开发机):把 Apple
   内核不允许/语义不同的指令改写掉 —— `x18`、`tpidr_el0` 读写改为线程槽
   访问;`svc`(系统调用)与 `cntpct_el0` 读取改为跳进宿主 C 代码的
   "host gate"。重写后的代码**在运行时永不再修改**,以满足 iOS 对可执行
   页的签名要求。
2. **宿主**(`dm/host/`,纯 C):扮演 Horizon 加载器 —— 把重写镜像映射进
   内存、按门表注册 gate、跨模块重定位,然后接管内核系统调用(HLE)、
   C 库函数和系统服务;每条游戏线程是宿主里的一个 pthread。
3. **图形层**(`dm/gfx/`):游戏用 NVN(Switch 图形 API)绘制。宿主强制
   接管唯一的入口 `nvnBootstrapLoader`,`gfx/nvn/hle_nvn.c`(C)把调用
   录成命令,`nvn_mtl.m` 把命令回放为 Metal;着色器离线翻译(Maxwell →
   SPIR-V(内嵌 Ryujinx)→ MSL)成 Metal 库。
4. **App 壳**(`dm/ios/`):iOS 前端(窗口、触控、手柄、资源下载),重写
   镜像经 `tools/gen_embed.py` 变成汇编编进签名代码段。

## 2. 为什么 HarmonyOS 是可行目标

- **同架构**:鸿蒙手机为 ARM64,与 Switch 同族,"原生重放而非模拟"的
  核心思路原样成立。
- **内核更宽松**:OHOS 内核是 Linux 系。Linux 用户态允许使用 `tpidr_el0`
  (标准 TLS)且可直接读 `cntvct_el0`;`x18` 在 Linux/OHOS 上不被内核
  征用(上游为 Apple 内核改写它,鸿蒙规则集大概率可以保留原指令)。
  **仍需改写**的是 `svc`(会陷入 Linux 内核,语义完全不同)与 Horizon
  线程局部区域(TLR)的访问 —— 走 host gate。
- **同样的代码签名约束**:OHOS 同样不允许运行时生成可执行页。上游
  `gen_embed.py` "把镜像编进签名代码段"的策略可原样复用:生成把
  `patched.bin` `.incbin` 进 "ax" 段的汇编,编进 `libdmhost.so`。
- **图形 API 可用**:OHOS NDK 自带 `libvulkan.so` 与 Vulkan 头,Vulkan
  能力覆盖 NVN 回放所需(计算、纹理压缩等按机型验证)。

## 3. 子系统映射总表

| # | 上游 | 作用 | HarmonyOS 对应 | 状态 |
|---|------|------|----------------|------|
| 1 | `dm/tools/patch_nso.py` | 离线指令重写器 | 复用;新增 `DM_RULESET=hos` 规则集(见 §4.1) | 待实施 |
| 2 | `dm/tools/*.py`(product.sh 等) | 构建产物脚本 | 复用(Python,平台无关,可在 Windows 跑) | 可复用 |
| 3 | `dm/host/loader.c` | 镜像映射/门表/重定位 | `cpp/loader/dm_guest_loader.*`(容器头 + 门表骨架) | 骨架就绪 |
| 4 | `dm/host/thread.c` 线程槽 | x18/TLS/TLR 替身 | `cpp/hal/dm_platform.*`:pthread_key 三槽 | 已完成 |
| 5 | 19.2MHz 计数器应答 | `cntpct_el0` HLE | `dmPlatCounterRead()`:`CLOCK_MONOTONIC` 换算 | 已完成 |
| 6 | `dm/host/hle*.c` | svc/线程/内存/服务 HLE | 纯 C 平移 + OHOS 适配(mmap/mprotect/pthread 均可用) | 待实施 |
| 7 | `dm/host/main.c` 启动序 | SDK 运行时 + init_array + nnMain | 并入 `libdmhost.so` 启动路径 | 待实施 |
| 8 | `dm/host/dm_*.c` 游戏特性 | 大秘境/Boss冲/易爆装备等 | 纯 C,依赖门表与镜像偏移,随 #3/#6 落地 | 待实施 |
| 9 | `dm/gfx/nvn/`(接口层 C) | NVN 函数表与命令录制 | 平移进 `cpp/gfx/` | 待实施 |
| 10 | `dm/gfx/nvn/nvn_mtl.m` | Metal 回放 | **Vulkan 回放后端**(新写) | 待实施 |
| 11 | `gfx/tools/` 着色器链 | Maxwell→SPIR-V→MSL | 止步 SPIR-V(Ryujinx Spv.Generator 产物),Vulkan 直接消费 | 待实施 |
| 12 | `dm/ios/Sources/front_ios.m` | iOS 前端接口 | `cpp/napi/dm_napi.cpp` + ArkUI(本仓库) | 已完成(演示) |
| 13 | CAMetalLayer 表面 | 画面输出 | XComponent + EGL/GLES3 循环(演示渲染) | 已完成(演示) |
| 14 | `DmTouchOverlay.m` + `dm_touch_controls.c` | 触控界面 | ArkUI 叠加层(摇杆/技能/腰带/Boss条)+ `cpp/input` | 已完成(演示) |
| 15 | CoreAudio | 音频 | OHAudio(NDK `libohaudio.so`),接 SDK 音频服务的 HLE | 待实施 |
| 16 | `dm/ios/RomfsDownloader` | 资源下载/落盘 | 应用沙箱 files 目录 + `dmPlatSetFilesDir` 已通 | 部分 |
| 17 | `dm/verify/`(Unicorn 回归) | 重写等价性验证 | 开发机侧复用,平台无关 | 可复用 |
| 18 | Xcode project.yml + setup.sh | 构建管线 | hvigor + CMake(`externalNativeOptions`,arm64-v8a) | 已完成 |

## 4. 关键技术决策

### 4.1 重写规则集:`DM_RULESET=hos`

上游按规则集产出重写产物(`keypage` / `rules` / `touch`)。移植需在
`patch_nso.py` 注册 `hos` 规则集,逐条对照:

| 指令 | Apple 规则集(现状) | 建议的 HOS 规则集 |
|------|--------------------|-------------------|
| `x18` 读写 | 改线程槽 | 大概率保留(Linux 不征用 x18;保守起见可沿用槽方案,先保留再实测) |
| `tpidr_el0` 读写 | 改线程槽 | 保留原指令(Linux TLS 语义兼容,需对齐 Horizon TCB 布局差异) |
| `tpidrro_el0` 读 | 改线程槽 | 改 gate(内核写入的是 Linux 语义值,Horizon 语义需宿主提供) |
| `svc #n` | 改 gate | **必须改 gate**(svc 会陷入 OHOS 内核) |
| `cntpct_el0` 读 | 改 gate | 可保留直读(Linux 允许),频率语义差异由宿主在 gate/SDK 层校准 |
| CUTS / OBS 表 | 按构建号注册 | 原样复用(与 OS 无关) |

验证:`dm/verify/regress.sh` 的 Unicorn 等价性试跑在开发机上即可对
`hos` 规则集执行,不需要设备。

### 4.2 代码段嵌入(签名约束)

OHOS 与 iOS 一样禁止运行时写可执行页。沿用上游方案:

```
宿主侧脚本(新增 dm/tools/hos_embed.py):
  <name>.patched.bin + <name>.gates.tsv
    → guest_<name>.S:
        .section .dm_guest_<name>, "ax"
        .incbin "<name>.patched.bin"
        <门表数据>
    → 并生成 DmGuestImageHeader(见 cpp/loader/dm_guest_loader.h)
CMake:把生成的 .S 加入 libdmhost.so 源列表
运行时:dmGuestLoadEmbedded() 直接登记,无需复制
```

本仓库的 `dm_guest_placeholder.S` 就是该格式的自检样例。

### 4.3 图形:NVN → Vulkan

上游 `nvn_mtl.m` 约 41 个 ObjC 文件回放 NVN 命令到 Metal。HOS 后端逐
职责对应改写为 Vulkan:

- NVN 命令缓冲/纹理/管线状态 → Vulkan command buffer / image / pipeline
- 上游离线把着色器翻到 MSL;HOS 直接用 Ryujinx 链产出的 **SPIR-V**,
  `vkCreateShaderModule` 消费,少一步翻译
- 纹理格式:A16 及更早芯片的补充纹理包(BC 压缩)在鸿蒙按机型 GPU
  (Maleoon/Adreno)验证 `VK_FORMAT_BC*` 支持,不支持时走上游
  `bcdec` 软解(`dm/third_party/bcdec` 可直接编入 native)
- 演示期表面:EGL/GLES3(本仓库 `cpp/render`)先行打通窗口/线程/输入
  通路,Vulkan 回放器就位后替换

### 4.4 触控界面

上游的触控规则("保留原界面语言,用手势替代额外按钮")原样适用于鸿蒙。
本仓库以 ArkUI 实现:摇杆(精确对角线)、技能键(点按/按住/拖拽瞄准)、
腰带、Boss 血条、小地图占位;合成输入经 NAPI 注入 `cpp/input`,与
XComponent 原生触摸事件汇入同一事件环。guest 接入后,这些事件改由
`dm_cursor.c` / `hle_private_aim.c` 平移版消费。

## 5. 实施路线(建议顺序)

1. **M1 重写规则集**:`patch_nso.py` 增加 `hos` 规则集;`regress.sh` 全
   绿(开发机,Unicorn)。
2. **M2 宿主平移**:`dm/host` 核心文件(loader/thread/hle_svc/hle_thread/
   entry.S 的 aarch64 汇编 gate 跳板)编进 `libdmhost.so`;macOS 专用
   路径替换为 OHOS 等价物。首里程碑:nnMain 能起、日志能打到 hilog。
3. **M3 图形后端**:NVN 录制层平移 + Vulkan 回放器;先出主菜单画面,
   再进游戏;着色器链接 SPIR-V。
4. **M4 音频/输入/资源**:OHAudio 接 SDK 音频服务 HLE;资源包落沙箱
   (`filesDir` 通路已就绪);触控/手柄映射接 `dm_padmap`。
5. **M5 特性回归**:`dm_*.c` 特性逐个打开,跑上游 `verify/accept.sh` 的
   等价 HOS 版。

## 6. 本仓库目录与上游对照

```
d2r-mobile-hos/
├── entry/src/main/cpp/
│   ├── hal/dm_platform.*        ← dm/host/thread.c 线程槽 + 计数器
│   ├── input/dm_input.*         ← dm/ios/Sources/dm_touch_controls.c 通路
│   ├── loader/dm_guest_loader.* ← dm/host/loader.c(骨架)+ gen_embed.py 格式
│   ├── loader/dm_guest_placeholder.S ← 内嵌镜像自检样例
│   ├── gfx/dm_gfx_backend.*     ← dm/gfx 后端接口 + Vulkan 探针
│   ├── render/dm_renderer.*     ← CAMetalLayer ⇢ XComponent+EGL/GLES3
│   └── napi/dm_napi.cpp         ← front_ios.m ⇢ NAPI 桥 + XComponent 回调
├── entry/src/main/ets/
│   ├── pages/{Index,Game,Settings,PortingStatus}.ets ← App 壳
│   ├── components/{VirtualStick,SkillButtons,BeltBar,BossBar}.ets
│   │                            ← DmTouchOverlay.m 的 ArkUI 版
│   └── common/DmNative.ets      ← dm_touch_game_bridge.c 的 ArkTS 版
└── docs/PORTING.md              ← 本文档
```
