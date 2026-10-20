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

### 4.1 重写规则集 `hos`(tools/patch_nso_hos.py,已交付)

上游按规则集产出重写产物(`native` / `rules` / `touch`)。HOS 规则集以独立
脚本 `tools/patch_nso_hos.py` 实现,CUTS/OBS/PARAMS 数据表直接从上游
`patch_nso.py` 源码切片提取,保持同步。

**核心差异 —— 槽基址**。上游补丁代码以 `mrs tpidrro_el0` 取 pthread TSD
数组基址(Apple 内核把 TSD 指针放进 tpidrro);Linux/OHOS 内核往 tpidrro_el0
写的是线程 TID(小整数),Apple 产物不能直接跑;x18 虽在 OHOS 上不被内核
征用,但它是客体自写自读的寄存器,宿主不能拿它当槽基址。HOS 方案:

- **门桩直连宿主**:镜像经 hos_embed.py 编进 libdmhost.so 的同一可执行段,
  每个门桩直接 `bl dm_gate`(汇编期符号解析),按 LR 返回点查门表 —— 与
  上游同一 ABI,但完全不需要每线程基址寄存器。
- **tpidr_el0 / tpidrro_el0 也走门**(新门类 tpidr_r / tpidr_w / tpidrro_r):
  宿主 C 侧用 pthread TSD 实现槽,绕开 musl 的 tpidr_el0(libc 依赖它,不
  能让客体覆盖)。
- **x18 不改写**:OHOS 内核与 musl 均不征用 x18,客体自写自读与 Horizon
  语义一致;`dm_gate` 保存并恢复 x18(Apple 版跳过恢复,因为内核会改它)。

| 指令 | Apple 规则集 | HOS 规则集(patch_nso_hos.py) |
|------|-------------|------------------------------|
| `x18` 读写 | 改线程槽 K18(含独占窗口/跳转表展开) | **保留**(不改写) |
| `mrs/msr tpidr_el0` | 内联读/写槽 KTLS | 门 tpidr_r / tpidr_w |
| `mrs tpidrro_el0` | 内联读槽 KTLR | 门 tpidrro_r |
| `svc #n` | 门(svc) | 门(svc),桩形一致 |
| `mrs cntpct/cntvct_el0` | 门 | 门,宿主按 19.2MHz 应答 |
| CUTS / OBS / PARAMS | 表驱动 | 同一张表(源码切片提取);PARAMS 变为 param 门 |

附带收益:HOS 规则集不需要 capstone(x18 的反汇编改写整个不需要),也不需
要 eh_frame 函数边界扫描(仅 OBS 入口断言用,暂略)。

产物格式与上游同名同列(`<名>.patched.bin/.gates.tsv/.sites.tsv/…`),另加
`<名>.gatebl.tsv`:镜像文件内必须由 hos_embed.py 换成 `bl dm_gate` 的字偏移
(重写器在这些位置留 `0x94000000` 占位)。

**验证**:`tools/test_patch_nso_hos.py` 构造合成 NSO(埋 svc/tpidr/cntpct/x18
指令),断言:埋点全换成跳转、x18 指令逐字保留、门数与 gatebl 数一致、
hos_embed 生成的 .S 含正确数量的 `bl dm_gate`。运行需 python3 + lz4;真实
模块的等价性回归仍用上游 `dm/verify/regress.sh`(开发机,平台无关)。

### 4.2 代码段嵌入(tools/hos_embed.py,已交付)

OHOS 与 iOS 一样禁止运行时写可执行页。沿用上游 gen_embed.py 策略并适配 ELF:

```
patch_nso_hos.py:
  <名>.patched.bin(含键页与补丁区)+ .gates.tsv + .gatebl.tsv
hos_embed.py:
  → guest_<名>.S:
      .section .dm_guest_<名>, "ax"        # 整块 4K 对齐,随 .so 签名
      DmGuestImageHeader v2(32B)
      .incbin 切片,gatebl 偏移处交替发 `bl dm_gate`
      容器门表(N × 32B:{ret_off, kind, arg, orig_off, rsv})
CMake:把生成的 .S 加入 libdmhost.so
运行时:dmGuestLoadEmbedded() 只登记基址与门,不复制代码
```

数据段写权限:容器整块在 "ax" 段,装载器在运行时对 `[DATA_OFF, 补丁区)` 页
mprotect 加写(Linux 系内核允许对自有映射改权限;若具体版本拒绝 RWX,则对
数据页去 X —— 数据页永不执行)。占位样例 `dm_guest_placeholder.S` 是该容器
格式的手写实例,镜像体是一个真实门桩,自检会实际穿过 `dm_gate` 执行。

### 4.3 宿主核心(cpp/host,已交付)

- `entry_hos.S`:上游 entry.S 的 ELF 移植 —— 记名桩表(4096)、`dm_gate`
  (全量保存/恢复 x0-x30+q0-q31+NZCV;**x18 亦恢复**,HOS 上它承载客体
  TCB)、`dm_thread_enter`(切客体栈调入口)。
- `hle_core.c`:门表(开地址哈希,按返回点一次命中)分派八类门(svc /
  cntpct / cntvct / obs / tpidr_r / tpidr_w / tpidrro_r / param);每线程
  环境(TLS 块 8K + Horizon TLR 0x200 + SDK 临界区线程对象 TLR[0x1f8] +
  参数位种子);未实现导入记名桩;HLE/HLE_FORCE 登记段(ELF `__start_swhle`
  边界迭代)。
- `hle_svc.c`:svc 分派框架 + 客体 4GiB 地址空间窗口(MAP_NORESERVE);每个
  未实现调号记一次 hilog,按清单排期。宿主私有调号(上游 hle.h 的
  SVC_HOST_*)占位。
- `host_boot.c`:启动序 —— 加载内嵌容器 → 校验 → 门表注册(绝对地址 =
  镜像基址 + ret_off)→ 每线程环境 → pthread 上 `dm_thread_enter` 调入口。
  真实三模块接入后扩展为上游 main.c 的 SDK 启动序(init_array、nnMain、
  跨模块重定位)。
- `front_hos.c`:前端接口子集(分辨率参数位),表面尺寸接入后由游戏页覆盖。

### 4.4 图形:NVN → Vulkan(cpp/gfx,骨架已交付)

上游 `nvn_mtl.m` 把 NVN 命令回放为 Metal。HOS 对应物:

- `gfx/nvn/dm_nvn.c`:NVN 引导接口骨架 —— 宿主侧 `nvnBootstrapLoader`
  返回函数表,表项为记名桩(首次调用记一行 hilog);录制层(hle_nvn.c
  平移)就位后逐个换成真实录制函数。
- `gfx/dm_vk.cpp`:Vulkan WSI 管线 —— 实例 → `VK_OHOS_surface`
  (`vkCreateSurfaceOHOS`,NDK 自 API 10 提供)→ 图形+呈现队列 → 交换链 →
  呈现循环。真机上 `useVulkan()` 成功即证明 Vulkan 出图通路可用;回放器
  就位后只换每帧命令(当前为清屏色动画)。
- 着色器链:上游 Maxwell→SPIR-V(Ryujinx)→MSL 两步;HOS 止步 SPIR-V,
  `vkCreateShaderModule` 直接消费,少一步翻译。
- 纹理:BC 压缩格式按机型 GPU 验证 `VK_FORMAT_BC*`,不支持时编入上游
  `dm/third_party/bcdec` 软解。
- EGL/GLES3 演示后端(`render/dm_renderer.cpp`)保留:同一表面可在两后端
  间切换,`devecocli run` 后在「移植状态」页可验证。

### 4.5 触控界面(已交付,见 entry/src/main/ets)

上游的触控规则("保留原界面语言,用手势替代额外按钮")原样适用于鸿蒙。
ArkUI 叠加层:摇杆(精确对角线)、技能键(点按/按住/拖拽瞄准 + 双技能组)、
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
