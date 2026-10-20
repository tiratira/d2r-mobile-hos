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
| **M2 宿主核心**:`host/entry_hos.S`(dm_gate 全量保存恢复)+ `hle_core.c`(八类门分派/线程环境/记名桩/HLE 段)+ `hle_svc.c`(分派框架+客体 VA 窗口)+ `host_boot.c`(加载→门注册→dm_thread_enter 启动序) | ✅ 编译通过,自检穿过 dm_gate 执行占位 guest |
| **M3 图形**:`gfx/dm_vk.cpp` — Vulkan 实例→`VK_OHOS_surface`→交换链→呈现循环;`gfx/nvn/` — NVN 引导函数表 + 记名桩 | ✅ 骨架编译通过(真机 `useVulkan()` 验证通路) |
| ArkUI:主菜单 / 游戏页(XComponent + 摇杆/技能/腰带/Boss条叠加层)/ 设置 / 移植状态(自检按钮 + VK/GLES 切换) | ✅ 已实现 |
| XComponent EGL/GLES3 演示渲染循环(与 Vulkan 后端可切换) | ✅ 已实现 |
| 真实游戏模块接入(重写→嵌入→SDK 启动序→NVN 录制回放→OHAudio) | ⏳ 下一阶段(需你的游戏文件) |

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
#    (加载三个真实容器 + 注册全部门表;nnMain 启动待 svc HLE 填齐)
```

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
