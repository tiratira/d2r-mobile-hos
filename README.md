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
| **M3 图形**:`gfx/dm_vk.cpp` — Vulkan 实例→`VK_OHOS_surface`→交换链→呈现循环;`gfx/nvn/` — NVN 引导函数表 + 记名桩 | ✅ 骨架编译通过(模拟器实测 Vulkan 实例 OK) |
| **资源管线**:`tools/extract_romfs_hos.py` — 基础版 RomFS 直读提取(7.68 GiB / 25 文件 CASC 档案);更新包 BKTR 补丁解密待破(见 P2) | ✅ base / ⏳ patch |
| ArkUI:主菜单 / 游戏页(XComponent + 摇杆/技能/腰带/Boss条叠加层)/ 设置 / 移植状态(自检按钮 + VK/GLES 切换) | ✅ 已实现 |
| XComponent EGL/GLES3 演示渲染循环(与 Vulkan 后端可切换) | ✅ 已实现 |
| 真实 nnMain 启动链(loader 重定位 / SDK init 序列 / fs·oe·hid HLE / NVN 回放 / OHAudio) | ⏳ P2 |

完整映射与实施路线见 **[docs/PORTING.md](docs/PORTING.md)**。

## 目录速览

```
tools/                M1:patch_nso_hos.py(hos 规则域)/ hos_embed.py(容器 v2 打包)/ test_patch_nso_hos.py
entry/src/main/cpp/
  host/               M2:entry_hos.S(门跳汇编)/ hle_core(门分派+线程环境)/ hle_svc / host_boot / front_hos
  gfx/                M3:dm_vk.cpp(Vulkan WSI 管线)/ nvn/(NVN 引导骨架)
  hal/ input/ loader/ render/ napi/   平台 HAL / 输入环 / 容器 v2 加载器 / EGL 演示 / NAPI 桥
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
# 5) 真机自检:「移植状态」页 → 宿主全链路自检
#    (加载三个真实容器 + 注册全部门表;svc HLE 自检 17/17)

# 6) RomFS 数据提取(基础版 NSZ 先解回 NSP,再从基础包直读;
#    产物 = 25 个文件 / 7.68 GiB:CASC 档案 00..17-00000001.data + 清单)
D:/Tools/nsz_v4.6.1_win64_portable/nsz.exe -D -o <基础NSP目录> "<基础.nsz>"
D:/Tools/nsz_v4.6.1_win64_portable/python.exe tools/extract_romfs_hos.py \
    <基础.nsp> <更新.nsp> <prod.keys> <RomFS输出目录> --base-only
```

## P2:真机 nnMain 启动链缺口(排期)

svc 内核层(内存/线程/同步,71 个调号)已全部落地并在模拟器实测通过;从
「三个真实模块加载完成」到「nnMain 跑起来」之间还差四块,均在 ARM64 真机上
才能端到端验证:

1. **loader 移植(上游 `loader.c`)** — 宿主扮演 Horizon rtld:把嵌入镜像拷进
   RWX 内存 → 解析 NSO 的 mod0/dynamic 段 → 应用重定位
   (`R_AARCH64_RELATIVE` + 跨模块 `JUMP_SLOT`/`GLOB_DAT`,导入按 `swhle`
   表接管,未实现导入落记名桩)→ 跑各模块 `DT_INIT_ARRAY`。
   注意:门表注册必须发生在**重定位之后**(返回点绝对地址随装载基址变)。
2. **SDK 启动序(上游 `main.c` 的 guest_main)** — 照 `nn::init::Start`
   原序:`_init_libc0 → nnosInitialize(hmain,0) → _init_libc1
   → nninitInitializeSdkModule → nninitStartup → _init_libc2 → 各模块
   DT_INIT → SetHostArgc/Argv → nnMain`。
3. **fs/oe/hid/time 等服务层 HLE**(上游 `hle_fs.c` 等 40+ 文件)— 先用记名
   桩日志驱动:真机跑一轮,hilog 的 UNIMPL 清单就是实现排期表;RomFS 读取
   走 `dm_data_pread` 服务上面提取的 CASC 档案。
4. **NVN 录制/回放 + OHAudio** — 图形与音频通路(上游 Metal 回放 → 本仓
   Vulkan 回放)。

**BKTR 补丁解密(开源问题)**:更新 NSP 的 RomFS 是基础包之上的 BKTR 叠加。
已逆向确认重定位条目为 20 字节 `{u64 虚拟, u64 物理, u32 isPatch}`(800/818
条合法、严格单调)与子段表布局,但补丁区 AES-CTR 计数器公式尚未破(试遍
offset 基准 × 21 个子段 CTR × nonce 变体,均未命中;nut/nsz 自身从不解密
BKTR 内容,无现成参考)。`--base-only` 先出基础版数据(v0,自洽完整);
要 v2228224 补丁数据,最快路径是 [hactoolnet](https://github.com/Thealexbarney/LibHac)
(上游 `extract_game.sh` 的原装工具,`hactoolnet --romfsdir` 直接解析 BKTR)。

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
「移植状态」页可视化了上游各子系统的移植进度。

## 上游致谢

- [d2r-mobile](https://github.com/d2r-mobile/d2r-mobile) 与其贡献者
  (MIT License)—— 重写器、宿主、图形层与验证框架的原始实现
- 其 third_party 中内嵌的 Ryujinx 着色器翻译组件等
