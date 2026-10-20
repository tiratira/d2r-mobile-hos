/*
 * PortingStatus — 上游 d2r-mobile 各子系统到 HarmonyOS 的移植状态一览。
 * 详细映射与实施路线见 docs/PORTING.md。
 */
import { router } from '@kit.ArkUI';
import { DmNative } from '../common/DmNative';

interface StatusRow {
  system: string;
  upstream: string;
  hos: string;
  state: string; // 已完成 / 骨架就绪 / 待实施
}

@Entry
@Component
struct PortingStatus {
  @State probe: string = '';
  private rows: StatusRow[] = [
    { system: '指令重写器', upstream: 'dm/tools/patch_nso.py(Python,离线)', hos: '复用;新增 DM_RULESET=hos 规则集', state: '待实施' },
    { system: '宿主加载器', upstream: 'dm/host/loader.c(Mach-O 映射)', hos: 'cpp/loader:内嵌镜像 + 门表骨架', state: '骨架就绪' },
    { system: '线程槽/计数器', upstream: 'dm/host/thread.c', hos: 'cpp/hal:pthread_key + 19.2MHz 单调时钟', state: '已完成' },
    { system: '系统调用接管', upstream: 'dm/host/hle*.c(svc gate)', hos: 'svc 改写 + OHOS 实现', state: '待实施' },
    { system: 'NVN 命令层', upstream: 'dm/gfx/nvn/hle_nvn.c(纯 C)', hos: '平移至 cpp/gfx', state: '待实施' },
    { system: '回放后端', upstream: 'dm/gfx/nvn/nvn_mtl.m(Metal)', hos: 'Vulkan 后端(NDK 已带 libvulkan)', state: '待实施' },
    { system: '着色器翻译', upstream: 'gfx/tools:Maxwell→SPIR-V→MSL', hos: '止步 SPIR-V,供 Vulkan 消费', state: '待实施' },
    { system: 'App 壳', upstream: 'dm/ios(Sources/*.m,UIKit)', hos: 'ArkUI + XComponent(本仓库 entry)', state: '已完成' },
    { system: '触控界面', upstream: 'DmTouchOverlay.m + dm_touch_controls.c', hos: 'ArkUI 叠加层 + cpp/input 通路', state: '已完成(演示)' },
    { system: '表面渲染', upstream: 'CAMetalLayer', hos: 'XComponent + EGL/GLES3 循环', state: '已完成(演示)' },
    { system: '音频', upstream: 'CoreAudio', hos: 'OHAudio(待接入)', state: '待实施' },
    { system: '构建管线', upstream: 'setup.sh + Xcode project.yml', hos: 'hvigor + CMake(本仓库)', state: '已完成' },
    { system: '验证框架', upstream: 'dm/verify(Unicorn 回归)', hos: '开发机侧复用,平台无关', state: '可复用' }
  ];

  aboutToAppear(): void {
    this.probe = DmNative.gfxProbe();
  }

  private stateColor(state: string): string {
    if (state.startsWith('已完成')) {
      return '#7fbf6a';
    }
    if (state === '骨架就绪' || state === '可复用') {
      return '#d8a53c';
    }
    return '#bf6a6a';
  }

  build() {
    Column({ space: 12 }) {
      Text('移植状态 · d2r-mobile → HarmonyOS')
        .fontSize(20)
        .fontColor('#c7a468')
        .fontWeight(FontWeight.Bold)
        .margin({ top: 20 })

      Scroll() {
        Column({ space: 8 }) {
          ForEach(this.rows, (row: StatusRow) => {
            Row() {
              Text(row.system)
                .fontSize(13)
                .fontColor('#e8d5a8')
                .width(110)
              Text(row.upstream)
                .fontSize(11)
                .fontColor('#9a8a6e')
                .layoutWeight(1)
              Text(row.hos)
                .fontSize(11)
                .fontColor('#b7a67e')
                .layoutWeight(1)
              Text(row.state)
                .fontSize(11)
                .fontColor(this.stateColor(row.state))
                .width(78)
                .textAlign(TextAlign.End)
            }
            .width('92%')
            .padding(8)
            .backgroundColor('rgba(30,20,10,0.55)')
            .borderRadius(8)
          }, (row: StatusRow) => row.system)

          Text(this.probe)
            .fontSize(11)
            .fontColor('#6d6250')
            .margin({ top: 6 })
        }
      }
      .layoutWeight(1)
      .scrollBar(BarState.Auto)

      Text('‹ 返回')
        .fontSize(14)
        .fontColor('#e8d5a8')
        .padding({ left: 14, right: 14, top: 6, bottom: 6 })
        .backgroundColor('rgba(30,20,10,0.72)')
        .borderRadius(8)
        .margin({ bottom: 14 })
        .onClick(() => {
          router.back();
        })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#0a0705')
    .alignItems(HorizontalAlign.Center)
  }
}
