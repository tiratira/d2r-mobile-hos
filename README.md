# D2R Mobile HOS · Diablo II: Resurrected for HarmonyOS(移植框架)

把 [d2r-mobile](https://github.com/d2r-mobile/d2r-mobile)(Switch 版《暗黑破坏神
II:重制版》在 iPhone/Mac 上的原生移植)的架构适配到 **HarmonyOS(鸿蒙)** 的
移植工程。核心思路与上游一致:**不做指令模拟,让重写后的 ARM64 游戏代码在
鸿蒙设备上原生执行** —— 鸿蒙手机与 Switch 同为 ARM64。

> **本仓库不包含任何游戏内容。** 与上游相同:不含游戏程序、美术、文本、数据
> 表或着色器;一切游戏内容须由使用者从自己拥有的游戏文件离线生成。仅限个人
> 学习与研究用途。

## 当前状态

| 层 | 状态 |
|----|------|
| HarmonyOS 工程骨架(hvigor + CMake,API 26,phone/tablet,横屏沉浸) | ✅ 可构建 |
| **M1 重写器**:`tools/patch_nso_hos.py` — `hos` 规则域(svc/tpidr/cntpct → 门桩直连 `dm_gate`,x18 保留;CUTS/OBS/PARAMS 表从上游源码切片同步) | ✅ 已交付(待真实 NSO 验证) |
| **M1 嵌入**:`tools/hos_embed.py` — 容器 v2 打包(`.incbin` 进签名段 + gatebl 替换 + 门表)+ 合成 NSO 单测 | ✅ 已交付 |
| **M2 宿主核心**:`host/entry_hos.S`(dm_gate 全量保存恢复)+ `hle_core.c`(八类门分派/线程环境/记名桩/HLE 段/模块注册表)+ `host_boot.c`(加载→门注册→dm_thread_enter 启动序) | ✅ 编译通过,自检穿过 dm_gate 执行占位 guest |
| **M2 svc HLE**:`hle_svc.c`(客体地址空间 + 内存/查询类)+ `hle_thread.c`(线程/锁/条件/地址仲裁/事件/WaitSync 全量)+ `hle_hostcall.c`(0x80 TACT zlib 解码 + 0x81-0x8b 转调原函数) | ✅ 模拟器自检 **17/17** |
| **M2 loader(rtld)**:`loader_hos.c` + 容器 v3 — 三模块**原地装载**(镜像在签名节内,仅数据窗口 mprotect RW 后原地重定位),符号哈希/跨模块解析/swforce 接管;模拟器实测 main 700 符号/6 万余重定位,启动序 9 符号全解析 | ✅ x86 data-only 同码验证 |
| **M2 SDK 启动序**:`host_start.c` — 照上游 `guest_main` 逐条:`_init_libc0 → nnosInitialize → _init_libc1 → nninitInitializeSdkModule → nninitStartup → _init_libc2 → 各模块 DT_INIT → SetHostArgc/Argv → nnMain`;ARM64 专起 8 MiB 客体主线程,x86 做预检 | ✅ 预检通过(9/9 符号,3 模块 1011 项 DT_INIT) |
| **M2 服务层 HLE**:`hle_fs.c`(24 条:rom/save/temp 挂载 + 路径解析 + 句柄表 + 全部文件/目录接口)、`hle_oe.c`(15 条:运行模式/焦点/通知队列/语言/设备号)、`hle_hid.c`(13 条:手柄状态 + 按下沿锁存 + 触摸屏)、`hle_time.c`(10 条:系统时钟/日历/本地时区)、`hle_misc.c`(14 条:aoc/err/ngc/nsd/pctl)、`hle_account.c`(21 条:本地用户/昵称/联网接口失败)、`hle_sdkinit.c`(7 条服务初始化强制接管)、`hle_net.c`(93 条:socket/nifm/ssl 离线化,Initialize 成功、联网调用失败) | ✅ 模拟器服务自检全绿;导入覆盖 **1148** 符号 → 模块 950 + 强制接管 192 + HLE 6,**0 记名桩** |
| **M3 图形**:`gfx/dm_vk.cpp` — Vulkan 实例→`VK_OHOS_surface`→交换链→呈现循环;`gfx/nvn/` — NVN 引导 + **512 槽静态 thunk 表**(每入口首调记名/参数)+ EGL/GLES3 双后端 | ✅ 模拟器实测 Vulkan 实例/双后端 OK |
| **M4 音频**:`audio/dm_audio.c` — OHAudio 48 kHz/2ch/S16LE 渲染器 + 写数据回调 + 800ms 正弦自检(异步线程,不阻塞 UI) | ✅ 模拟器实测创建/播放完成 |
| **资源管线**:基础版直读 `extract_romfs_hos.py --base-only`(IVFC L4 逐块认证)+ v2228224 完整提取 `extract_romfs_hactool.sh`(基座区 100% 块一致交叉验证) | ✅ 25 文件 7.68 GiB + 46 文件 8.36 GiB |
| ArkUI:主菜单 / 游戏页(XComponent + 摇杆/技能/腰带/Boss条叠加层)/ 设置 / 移植状态(启动预检、服务自检、音频自检、播放音、宿主全链路、VK/GLES 切换) | ✅ 已实现 |
| 真实 nnMain 启动链(loader 重定位 / SDK init 序列 / fs·oe·hid HLE / NVN 回放 / OHAudio) | ✅ **真机 Mate 80(arm64,API 26)实测跑通到 nnMain**;游戏数据导入后画面仍待 NVN 回放接入 |

完整映射与实施路线见 **[docs/PORTING.md](docs/PORTING.md)**。

## 真机(ARM64)运行与游戏数据导入

x86_64 模拟器只能跑宿主框架层(x86 构建里客体镜像是纯数据,不执行 ARM64),
真实 `nnMain` 必须在 ARM64 鸿蒙设备上跑:

```bash
devecocli device list                  # 找真机 serial,例如 62T0226211000381
devecocli run --device <serial>        # 装包并启动;进「进入游戏」自动走启动链
```

游戏数据(RomFS)不进仓库,需要推到应用沙箱(8.36 GiB 完整集,约 46 个文件):

```bash
# 本机先按 docs/PORTING.md / README「重写自己游戏文件的管线」提取到本地目录
hdc -t <serial> file send -b com.d2rmobile.hos <本地romfs目录> \
    /data/storage/el2/base/haps/entry/files/romfs
```

之后重进游戏页,顶部会显示 `BOOT <stage>`(SDK 启动序)与导入覆盖统计;
真机上用 `hdc shell hilog | grep D2RMobileHOS` 可看到 `BOOT/UNIMPL/CRASH`
以及崩溃现场(寄存器 + 客体帧链)。

## 最后阶段(P2 收口)交付内容

这一轮把 README 里原「P2 真机 nnMain 启动链缺口」的四块全部落到代码里,
并在 x86_64 模拟器上把可离线验证的部分跑成回归:

1. **SDK 启动序** — `host/host_start.c`
   - 预检(两种 ABI):三模块装载 / 启动序 9 符号 / DT_INIT 模块与条目数 /
     门表 / 服务接管覆盖 / 未解析导入桩清单;
   - ARM64:起 8 MiB 栈的客体主线程,按 `nn::init::Start` 原序执行到 `nnMain`,
     每步打 `BOOT <stage>` 到 hilog,`dmHostBootStage()` 可轮询。
2. **服务层 HLE** — 6 个新文件、106 条 `HLE_FORCE` 强制接管;导入覆盖报告
   (`dm_import_report`)把「模块内定义 / 强制接管 / HLE / 弱 / 记名桩」分类,
   真机上「落记名桩」清单就是下一批接口的排期表。
3. **NVN 接口层** — `dm_nvn_stubs.S` 512 槽静态 thunk + 引导表按名登记:
   真实镜像跑起来后每个 NVN 入口第一次调用会打一行 `NVN <名字>`(带 x0/x1/x2),
   录制/回放逐槽替换;呈现端是 Vulkan WSI 清屏循环。
4. **OHAudio** — 48 kHz/2ch/S16LE 渲染器 + 写数据回调 + 正弦自检;游戏
   `nn::audio` 的 `AudioOutBuffer` 语义与 OHAudio 回调同构,后续按同一渲染器
   投递 PCM。

### 模拟器验证(「移植状态」页 → 按钮)

| 按钮 | 期望输出 |
|------|----------|
| 启动预检 | `3 模块 | 启动序符号 9/9 | DT_INIT 3 模块/1011 项 | 门 352 | 接管 HLE 7/force 106 | imp:1148 模块1043 接管99 HLE6 弱0 桩0 | x86_64:预检通过...` |
| 服务自检 | `fs: ...写/读/大小=OK 列目录=OK 改名=OK 删除=OK | oe: 模式=0 焦点=1 通知队列=OK | hid: ... OK | time: ... ToCalendarTime=OK | misc: aoc=1 ... profanity=OK | account: 本地用户=OK nick=Player nsa=不可用(预期)` |
| 音频自检 | `ohaudio: 48kHz/2ch/S16LE 渲染器创建 OK` |
| 播放音 | `playTone(800ms) -> 0 | ohaudio: 正弦 800ms 播放完成` |
| 宿主全链路自检 | `3 modules | ... | boot syms 7/7 | boot: guest 执行仅限 ARM64 真机... | svc-hle: 17/17` |

> x86_64 模拟器跑的是 `guest_*_x86.S` data-only 镜像:装载/重定位/符号解析/
> 预检与真机同码,但不执行 ARM64 客体指令。真实 `nnMain` 需要 ARM64 鸿蒙
> 设备(或等价的 ARM64 运行时)才能端到端验证 —— 这是本阶段唯一仍未闭环的
> 验证项,代码路径已全部就位。

## 目录速览

```
tools/                M1:patch_nso_hos.py(hos 规则域)/ hos_embed.py(容器 v3 打包)/ test_patch_nso_hos.py
                      imports_report.py(三模块导入解析覆盖:HLE 排期表)
entry/src/main/cpp/
  host/               entry_hos.S(dm_gate)/ hle_core(门分派+线程环境+导入覆盖)
                      hle_svc / hle_thread / hle_hostcall(svc HLE)
                      loader_hos.c(rtld)/ host_boot.c / host_start.c(SDK 启动序)
                      hle_libc.c / hle_sdkinit.c
                      hle_fs.c / hle_oe.c / hle_hid.c / hle_time.c / hle_misc.c / hle_account.c
  gfx/                dm_vk.cpp(Vulkan WSI 管线)/ nvn/(dm_nvn.c + dm_nvn_stubs.S 512 槽)
  audio/              dm_audio.c(OHAudio 渲染器 + 正弦自检)
  hal/ input/ loader/ render/ napi/   平台 HAL / 输入环 / 容器 v3 加载器 / EGL 演示 / NAPI 桥
entry/src/main/ets/   ArkUI:pages(Index|Game|Settings|PortingStatus) + components(触控层)
docs/PORTING.md       上游 → 鸿蒙 逐子系统映射与路线图
../d2r-mobile-src/    上游源码(克隆自 github.com/d2r-mobile/d2r-mobile)
```

## 重写自己游戏文件的管线(本机已验证到第 5 步)

```bash
# 0) Python 环境(uv)+ ExeFS 提取(nsz 便携包的解密栈,无需 hactool;
#    密钥只在本机读取;已核对官方 3.3.94012 的 sha256)
uv venv .venv && uv pip install --python .venv lz4
D:/Tools/nsz_v4.6.1_win64_portable/python.exe tools/extract_exefs_hos.py \
    <更新.nsp> <prod.keys> dm-work/data/switch/3.3.94012/Update/ExeFS

# 1) 合成 NSO 快速自检
.venv/Scripts/python.exe tools/test_patch_nso_hos.py

# 2) HOS 规则域重写三模块(main: 179 门/22 剪口;sdk: 173 门;subsdk0: 原样)
for m in main sdk subsdk0; do \
  .venv/Scripts/python.exe tools/patch_nso_hos.py \
    dm-work/data/switch/3.3.94012/Update/ExeFS/$m dm-work/build/hos/$m $m; done

# 3) 打包成嵌入汇编(guest_*.S 已被 .gitignore 排除:含重写后的游戏代码)
for m in main sdk subsdk0; do \
  .venv/Scripts/python.exe tools/hos_embed.py --outdir dm-work/build/hos/$m \
    --name $m --kind $([ $m = main ] && echo 0 || ([ $m = sdk ] && echo 1 || echo 2)) \
    --out entry/src/main/cpp/loader/guest_$m.S; done

# 4) 构建(CMake 自动探测 guest_*.S 并嵌入)
devecocli build
# 5) 真机/模拟器自检:「移植状态」页 → 启动预检 / 服务自检 / 音频自检

# 6) RomFS 数据提取(两条路径,均已本机验证):
#    a) 基础版直读(自研,tools/extract_romfs_hos.py --base-only;对 NCA 内 IVFC
#       L4 哈希层逐块认证):25 文件 7.68 GiB
#    b) v2228224 完整 BKTR 解析(hactool,tools/extract_romfs_hactool.sh;
#       基座区与认证基础版 100% 块一致):46 文件 8.36 GiB,含游戏要的 18 个 .data
D:/Tools/nsz_v4.6.1_win64_portable/python.exe tools/extract_romfs_hos.py \
    <基础.nsp> <更新.nsp> <prod.keys> <目录> --base-only
HACTOOL=D:/Tools/hactool/hactool.exe tools/extract_romfs_hactool.sh \
    <基础.nsp> <更新.nsp> <prod.keys> <目录>
# 产物放到沙箱 files/romfs;存档目录 files/save(DM_ROMFS/DM_SAVE 可覆盖)
```

## P2 收口状态

| 原 P2 项 | 状态 |
|----------|------|
| 1. loader 移植(原地重定位/符号解析/DT_INIT) | ✅ 完成(x86 同码验证) |
| 2. SDK 启动序(`host_start.c`) | ✅ 代码完成,预检通过;真机执行待验 |
| 3. fs/oe/hid/time 服务层 HLE | ✅ 完成最小集 + misc/account;0 未解析桩 |
| 4. NVN 录制/回放 + OHAudio | 🔶 NVN 接口/记名 thunk + Vulkan WSI、OHAudio 播放已通;**着色器翻译与完整 NVN 命令回放**仍待(见下) |

剩余工作(按优先级):

1. **ARM64 真机跑一轮启动链**:看 hilog 的 `BOOT` 阶段与 `UNIMPL` 清单,按清单
   补 fs/oe/hid 的真实语义(当前是最小集)与 account/friends 等 P2 接口。
2. **NVN 命令录制 + Vulkan 回放**:按 `NVN <名字>` 首调清单逐槽实现;着色器
   链止步 SPIR-V(Maxwell→SPIR-V 复用上游 Ryujinx 组件),`vkCreateShaderModule`
   直接消费。
3. **存档事务**:上游的 clonefile + RENAME_SWAP 在 OHOS 用「两次 rename」替换,
   并保留「本事务有写失败就拒绝 Commit」的 poison 语义。
4. **P2 服务接口**:friends 23 条 / swkbd(中文起名)/ codec(Opus)/ charname
   (中文名校验)等,按真机 UNIMPL 清单排期。

## 构建与运行

环境:DevEco Studio(含 SDK/NDK)与 `devecocli`(见 deveco-cli skill)。

```bash
# 静态检查
devecocli check arkts

# 构建(产物为 entry-default-signed.hap)
devecocli build

# 连接设备/模拟器后安装运行
devecocli run
```

应用首页为主菜单;「进入游戏」打开全屏游戏页 —— native 演示渲染在底层,
摇杆/技能/腰带等触控层悬浮其上,合成输入实时注入 native 输入环;
「移植状态」页可视化了上游各子系统的移植进度,并提供启动预检/服务自检/
音频自检/播放音等回归按钮。

## 上游致谢

- [d2r-mobile](https://github.com/d2r-mobile/d2r-mobile) 与其贡献者
  (MIT License)—— 重写器、宿主、图形层与验证框架的原始实现
- 其 third_party 中内嵌的 Ryujinx 着色器翻译组件等
