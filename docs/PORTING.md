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
| 1 | `dm/tools/patch_nso.py` | 离线指令重写器 | 复用;新增 `DM_RULESET=hos` 规则集(见 §4.1) | 已完成 |
| 2 | `dm/tools/*.py`(product.sh 等) | 构建产物脚本 | 复用(Python,平台无关,可在 Windows 跑) | 可复用 |
| 3 | `dm/host/loader.c` | 镜像映射/门表/重定位 | `cpp/loader/dm_guest_loader.*` + `host/loader_hos.c`(原地重定位/符号哈希/DT_INIT) | 已完成 |
| 4 | `dm/host/thread.c` 线程槽 | x18/TLS/TLR 替身 | `cpp/hal/dm_platform.*`:pthread_key 三槽 | 已完成 |
| 5 | 19.2MHz 计数器应答 | `cntpct_el0` HLE | `dmPlatCounterRead()`:`CLOCK_MONOTONIC` 换算 | 已完成 |
| 6 | `dm/host/hle*.c` | svc/线程/内存/服务 HLE | svc/线程/同步 17/17;服务层 `hle_fs/oe/hid/time/misc/account/sdkinit/net` 最小集(见 §4.6) | 已完成(最小集) |
| 7 | `dm/host/main.c` 启动序 | SDK 运行时 + init_array + nnMain | `host/host_start.c`:`_init_libc0→…→nnMain`(见 §4.3) | 已完成;**真机 Mate 80 (arm64) 实测跑通到 nnMain** |
| 8 | `dm/host/dm_*.c` 游戏特性 | 大秘境/Boss冲/易爆装备等 | 纯 C,依赖门表与镜像偏移,随 #3/#6 落地 | 待实施 |
| 9 | `dm/gfx/nvn/`(接口层 C) | NVN 函数表与命令录制 | `cpp/gfx/nvn`:引导表 + 512 槽 thunk + 首调记名(见 §4.7) | 骨架就绪 |
| 10 | `dm/gfx/nvn/nvn_mtl.m` | Metal 回放 | **Vulkan 回放后端**(`dm_vk.cpp` WSI 已通,命令回放待录制层) | 部分 |
| 11 | `gfx/tools/` 着色器链 | Maxwell→SPIR-V→MSL | 止步 SPIR-V(Ryujinx Spv.Generator 产物),Vulkan 直接消费 | 待实施 |
| 12 | `dm/ios/Sources/front_ios.m` | iOS 前端接口 | `cpp/napi/dm_napi.cpp` + ArkUI(本仓库) | 已完成(演示) |
| 13 | CAMetalLayer 表面 | 画面输出 | XComponent + EGL/GLES3 循环(演示渲染) | 已完成(演示) |
| 14 | `DmTouchOverlay.m` + `dm_touch_controls.c` | 触控界面 | ArkUI 叠加层(摇杆/技能/腰带/Boss条)+ `cpp/input` | 已完成(演示) |
| 15 | CoreAudio | 音频 | `audio/dm_audio.c`:OHAudio 48kHz/2ch/S16LE 渲染器 + 正弦自检;SDK 音频服务 HLE 待接 | 已实现(演示) |
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

### 4.6 SDK 启动序 + 服务层 HLE(host_start.c / hle_*.c,已交付)

**启动序**(`host/host_start.c`),照上游 `dm/host/main.c` 的 `guest_main`
逐条:

```
_init_libc0 → nnosInitialize(hmain,0) → _init_libc1
→ nninitInitializeSdkModule → nninitStartup → _init_libc2
→ 各模块 DT_INIT(rtld InitModules 角色)→ SetHostArgc/Argv → nnMain
```

- 注释里的「rtld 传入的三个回调」:A 只置 rtld 标志(略)、B = 各模块
  `DT_INIT`(`loader_hos.c` 的 `run_module_inits`)、C 析构(不做)。
- `nnosInitialize` 的 x0 是主线程句柄(`dm_svc_thread_main_init()`),
  上游注释指出 SDK 的 `ThreadManagerImplByHorizon` 会取用。
- x86_64 构建(PC 模拟器)只做**预检**:三模块装载/9 个启动序符号/
  DT_INIT 模块与条目数/门表/服务接管覆盖/未解析导入桩;ARM64 构建才起
  8 MiB 栈的客体主线程真正执行,每步 `BOOT <stage>` 打到 hilog,
  `dmHostBootStage()` 供 UI 轮询。

**服务层 HLE**(`hle_sdkinit/fs/oe/hid/time/misc/account.c`)。Horizon 的
系统服务在宿主里不存在,SDK 里的服务客户端若真去 IPC 会挂死,因此用
`HLE_FORCE` 在**重定位时强制接管**(优先于模块内定义):

| 文件 | 条目 | 内容 |
|------|------|------|
| `hle_sdkinit.c` | 7 | nndiag/oe/aoc/err/fs/pctl/diag 的 Initialize → 空操作 |
| `hle_fs.c` | 24 | rom/save/temp 挂载 + 路径解析(挂载名/`..` 防逃逸)+ 句柄表(fd 下标+1)+ 全部文件/目录接口;P0 存档事务 = fsync |
| `hle_oe.c` | 15 | 运行模式(手机=掌机)/焦点/性能模式/系统通知队列(15/30/31)/语言/伪设备号 |
| `hle_hid.c` | 13 | Npad 状态 0x28 字节结构 + 按下沿锁存 + 触摸屏状态;`dmHidSet` 供触控层注入 |
| `hle_time.c` | 10 | 系统时钟/日历(localtime_r)/本地时区;`time` 转调 SDK(开机时长语义) |
| `hle_misc.c` | 14 | aoc(拥有 1 个 DLC)/err(只打日志)/ngc(profanity 掩码写 0)/nsd/pctl |
| `hle_account.c` | 21 | 本地用户模型(Uid/UserHandle 魔数)/昵称;联网账号接口失败 |
| `hle_net.c` | 93 | socket/nifm/ssl 离线化:Initialize 成功不建会话,联网调用按 BSD 回 -1/ENETDOWN;`nn::socket::Initialize` 必须接管,否则 SDK 自己走 hipc 连 `sm:` → diag::Abort |
| `hle_vi.c` | 4 | nn::vi 显示层:Initialize/OpenDefaultDisplay/CreateLayer/GetNativeWindow → 宿主静态对象,不走 vi 服务 |
| `hle_friends.c` | 26 | friends 服务全剪:好友/资料/游玩记录/通知/邀请/在线状态;Result 失败、列表为空、事件永不触发 |
| `hle_libc.c` | 6 | getentropy、异常栈查询、`__rel_dyn/plt_*` 数据符号 |

**导入覆盖报告**(`dm_import_report`,启动预检里显示):装载/重定位后扫
三模块 dynsym,把每个未定义导入按解析去向分类。当前 3.3.94012 三模块实测:

```
imp:1148 模块950 接管192 HLE6 弱0 桩0
```

即**没有任何导入落到记名桩**。真机上若出现 `UNIMPL <name>` 或桩计数增加,
那份清单就是下一批要实现的接口排期表。

**限制**(有意为之的最小集):存档未做 clonefile 事务(上游用
`clonefile+renamex_np(RENAME_SWAP)`,OHOS 需换两次 rename);fs 的 overlay/
`.holes` 剔零档案、friends/swkbd/codec/charname 等属 P2;hid 还没有物理手柄
通路(触控层经 `dmHidSet` 注入)。

#### 4.6.1 真机(ARM64)落地记录(HUAWEI Mate 80,VYG-AL00,arm64-v8a/API 26)

x86_64 模拟器上客体镜像是纯数据(`data-only`),无法暴露 ARM64 执行问题。
第一次上真机时「进入游戏」立即闪退,定位到三个只在真机出现的缺陷(均已修):

1. **门桩栈失衡** — `patch_nso_hos.py` 里 `STP_PRE_2930` 写的是
   `stp x29,x30,[sp,#-32]!`(0xA9BE7BFD),而桩尾是 `ldp [sp],#16`;
   每次门调用净漏 16 字节栈,几千次后 `ret` 跳到垃圾地址
   (crash: `SIGSEGV@0`,`pc=0 lr=0`)。改为 0xA9BF7BFD(-16),单测断言
   `stp/ldp/bl` 三条编码。
2. **隐式函数声明截断** — `loader_hos.c` 调用 `resolve_import()` 时没有
   原型,Clang 按 `int` 处理,HLE 函数地址被截成 32 位;SDK 在
   `nnosInitialize → RngManager` 里 `br` 到 `0x7d19ed4` 崩溃。把原型加进
   `dm.h`,并把 `-Werror=implicit-function-declaration` 加进 CMake。
3. **挂载名丢失** — `MountRom/MountTemporaryStorage/MountSaveData` 忽略
   游戏传来的挂载名(实测 `$` = rom、`^` = 临时区),`^:/Diablo II/GameLogs`
   找不到挂载点被当只读,游戏初始化提前退出。改为按 guest 名登记宿主目录。

配套:全进程 `crash_hos.c`(SIGSEGV/ABRT 时把启动阶段、通用寄存器、客体
x29 帧链以 FATAL 打到 hilog)、`LOG_TAG` 修正(之前为 NULL,greppable 日志
看不到)、客体入口 `x18=0`(Horizon 新线程初值)。

真机数据(RomFS,8.36 GiB)不进仓库,用 hdc 的沙箱模式推入:

```bash
hdc -t <serial> file send -b com.d2rmobile.hos <本地romfs目录> \
    /data/storage/el2/base/haps/entry/files/romfs
```

实测启动链:`_init_libc0 → nnosInitialize → _init_libc1 →
nninitInitializeSdkModule → nninitStartup → _init_libc2 → DT_INIT
(main 852 + sdk 115 + subsdk0 44 项) → SetHostArgc/Argv → nnMain`,
随后 game 创建 6 个线程并挂载 `$`/`^` 文件系统。**下一次缺口**:NVN
函数表目前是记名 thunk(不录命令),`nnMain` 返回后画面仍黑;接入
NVN 命令录制 → Vulkan 回放后才有游戏画面。

### 4.7 NVN 接口层与 OHAudio(audio/dm_audio.c,已交付)

- `gfx/nvn/dm_nvn.c` + `dm_nvn_stubs.S`:宿主 `HLE_FORCE` 接管
  `nvnBootstrapLoader`;按名字登记入口并返回 **512 槽静态 thunk**
  (ARM64 汇编,`movz x9,#槽号; b 入口`),每个入口第一次调用打一行
  `NVN <名字>`(带 x0/x1/x2),这就是录制层的落点排期表。thunk 不生成
  运行时代码,符合 .so 签名约束。
- **真机推进(3.3.94012)**:`nvnDeviceGetProcAddress` 已换成真实宿主函数
  (`dmNvnInit` 登记 + proc 分发),真机第一次进入游戏即解析出 **128 个
  NVN 入口**(清单见 [nvn_capture.md](nvn_capture.md));未实现的入口返回
  统一非空占位(不再跳 0)。随后补了 device/queue/window/commandBuffer/
  memoryPool/program/texturePool/samplerPool 的最小宿主对象模型,引擎可以
  走完 NVN device 初始化、资源池、program 编译和 queue builder 设置。
- 当前最后一道墙:引擎在某个 guest 对象的 **`+0xE8`** 处取一张函数表
  (`ldr x1,[x0,#232]; ldr x8,[x1,#16]; 24 字节步长索引;尾调用`),该表为空
  导致 NULL deref。`host/crash_hos.c` 现在会 dump callee-saved 对象及其
  `[0..24]`/`+0xE8`,下一个里程碑就是按上游 `nvn_obj.h` 的对象契约把这张
  表补出来(然后才能录命令、present)。
- 旁路服务已补齐:`hle_vi.c`(显示层 4 条)、`hle_friends.c`(好友/在线/
  邀请/游玩记录 26 条,真机实测挡住 `friends::GetPlayHistoryRegistrationKey
  → sm:` 的 diag::Abort)、`hle_net.c`(socket/nifm/ssl 离线化)、
  `nv::InitializeGraphics` 等 SDK NVN 驱动入口强制空操作。
- `gfx/dm_vk.cpp`:Vulkan WSI(实例→`VK_OHOS_surface`→交换链→呈现)已
  在模拟器实测通过;录制层就位后逐槽换成真实命令,呈现端只换每帧命令
  (当前为清屏色动画)。着色器链止步 SPIR-V。
- `audio/dm_audio.c`:OHAudio 48 kHz/2ch/S16LE 渲染器 + 写数据回调
  (`audioDataSize` 按字节、整帧填满才返 VALID)+ 800 ms 正弦自检;
  Start/Stop/Release 在独立线程执行(Stop 可能 >50 ms,不能占 UI 线程)。
  游戏 `nn::audio` 的 AudioOutBuffer「填一块/交一块/取回已播块」语义与
  OHAudio 回调同构,后续按同一渲染器投递 PCM。

## 5. 实施路线(当前进度)

1. **M1 重写规则集**:`patch_nso_hos.py` 已交付;合成 NSO 单测 +
   对三模块真实产物的门数/剪口/重定位计数均已核对。 ⟶ 已完成
2. **M2 宿主平移**:loader/svc/线程/启动序/服务层最小集已编进
   `libdmhost.so`;x86_64 模拟器跑通装载→预检→服务自检。 ⟶ 已完成
   (真实 `nnMain` 执行需要 ARM64 鸿蒙设备)
3. **M3 图形后端**:NVN 接口层 + 512 槽记名 thunk + Vulkan WSI 已通;
   命令录制/回放与着色器链(Maxwell→SPIR-V)待做。 ⟶ 进行中
4. **M4 音频/输入/资源**:OHAudio 渲染器 + 正弦自检已通;资源包落沙箱
   (`filesDir` 通路 + fs HLE)已通;触控/手柄映射待接 `dm_padmap`/
   `dmHidSet`。 ⟶ 进行中
5. **M5 特性回归**:`dm_*.c` 特性逐个打开,跑上游 `verify/accept.sh` 的
   等价 HOS 版;依赖真机 UNIMPL 清单驱动。 ⟶ 待真机

## 6. 本仓库目录与上游对照

```
d2r-mobile-hos/
├── entry/src/main/cpp/
│   ├── hal/dm_platform.*        ← dm/host/thread.c 线程槽 + 计数器
│   ├── input/dm_input.*         ← dm/ios/Sources/dm_touch_controls.c 通路
│   ├── loader/dm_guest_loader.* ← dm/host/loader.c 容器/加载 + gen_embed.py 格式
│   ├── loader/dm_guest_placeholder.S ← 内嵌镜像自检样例
│   ├── host/hle_core.c          ← dm/host/hle.c(门分派/线程环境/导入覆盖)
│   ├── host/hle_svc.c hle_thread.c hle_hostcall.c ← dm/host/hle_svc.c 等
│   ├── host/loader_hos.c        ← dm/host/loader.c(rtld:重定位/符号/DT_INIT)
│   ├── host/host_boot.c         ← 容器冒烟 + 模块装载探针
│   ├── host/host_start.c        ← dm/host/main.c guest_main(SDK 启动序)
│   ├── host/hle_sdkinit.c       ← dm/host/hle_sdkinit.c
│   ├── host/hle_fs.c hle_oe.c hle_hid.c hle_time.c hle_misc.c hle_account.c
│   │                            ← dm/host/hle_*.c 的服务层最小集
│   ├── host/hle_libc.c          ← dm/host/hle_os.c 的 getentropy 等 + rel 标记
│   ├── gfx/dm_gfx_backend.*     ← dm/gfx 后端接口 + Vulkan 探针
│   ├── gfx/dm_vk.cpp            ← dm/gfx/nvn/nvn_mtl.m ⇢ Vulkan WSI
│   ├── gfx/nvn/dm_nvn.c dm_nvn_stubs.S ← dm/gfx/nvn/hle_nvn.c 接口层(512 槽)
│   ├── audio/dm_audio.c         ← CoreAudio ⇢ OHAudio
│   ├── render/dm_renderer.*     ← CAMetalLayer ⇢ XComponent+EGL/GLES3
│   └── napi/dm_napi.cpp         ← front_ios.m ⇢ NAPI 桥 + XComponent 回调
├── entry/src/main/ets/
│   ├── pages/{Index,Game,Settings,PortingStatus}.ets ← App 壳
│   ├── components/{VirtualStick,SkillButtons,BeltBar,BossBar}.ets
│   │                            ← DmTouchOverlay.m 的 ArkUI 版
│   └── common/DmNative.ets      ← dm_touch_game_bridge.c 的 ArkTS 版
└── docs/PORTING.md              ← 本文档
```
