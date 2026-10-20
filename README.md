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
| Native 宿主库 `libdmhost.so`(平台 HAL / 输入 / guest 加载器骨架 / 图形后端接口) | ✅ 编译通过 |
| XComponent + EGL/GLES3 原生渲染循环(表面/线程/触摸通路演示) | ✅ 已实现 |
| ArkUI 触控叠加层:虚拟摇杆、技能键(点按/按住/拖拽瞄准)、腰带、Boss 血条、小地图占位 | ✅ 已实现(演示态) |
| guest 镜像内嵌加载 + 门表自检(等价上游 gen_embed.py 策略) | ✅ 骨架 + 自检样例 |
| 指令重写器 `hos` 规则集(svc gate / TLR) | ⏳ 待实施 |
| NVN → Vulkan 图形回放后端 | ⏳ 待实施(NDK 已带 libvulkan) |
| 宿主 HLE(syscall/线程/内存/服务)平移 | ⏳ 待实施 |

完整映射表与实施路线见 **[docs/PORTING.md](docs/PORTING.md)**。

## 目录速览

```
entry/src/main/cpp/   native 宿主:hal / input / loader / gfx / render / napi
entry/src/main/ets/   ArkUI:pages(Index|Game|Settings|PortingStatus) + components(触控层)
docs/PORTING.md       上游 → 鸿蒙 逐子系统映射与路线图
../d2r-mobile-src/    上游源码(克隆自 github.com/d2r-mobile/d2r-mobile)
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
