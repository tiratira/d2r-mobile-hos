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
  @State bootReport: string = '未运行';
  private rows: StatusRow[] = [
    { system: '指令重写器', upstream: 'dm/tools/patch_nso.py(Python,离线)', hos: 'tools/patch_nso_hos.py:hos 规则域(门桩直连 dm_gate)', state: '骨架就绪' },
    { system: '镜像嵌入', upstream: 'ios/tools/gen_embed.py(.incbin 进签名段)', hos: 'tools/hos_embed.py:容器 v2 + gatebl 替换', state: '已完成' },
    { system: '宿主加载/门注册', upstream: 'dm/host/loader.c', hos: 'cpp/loader v3 + host/loader_hos.c(原地装载/重定位/符号哈希)', state: '已实现' },
    { system: '门分派汇编', upstream: 'dm/host/entry.S(Mach-O)', hos: 'cpp/host/entry_hos.S(ELF,x18 保持)', state: '已完成' },
    { system: '线程槽/计数器', upstream: 'dm/host/thread.c + hle.c', hos: 'cpp/hal + hle_core(pthread TSD + 19.2MHz)', state: '已完成' },
    { system: '系统调用接管', upstream: 'dm/host/hle_svc.c / hle_thread.c', hos: '内存/查询类 + 线程/同步类 + 0x80 起宿主调用(zlib/转调),自检 17/17', state: '已实现' },
    { system: 'NVN 命令层', upstream: 'dm/gfx/nvn/hle_nvn.c(纯 C)', hos: 'cpp/gfx/nvn:引导 + 函数表 + 记名桩', state: '骨架就绪' },
    { system: '回放后端', upstream: 'dm/gfx/nvn/nvn_mtl.m(Metal)', hos: 'cpp/gfx/dm_vk:实例→VK_OHOS_surface→交换链→呈现', state: '骨架就绪' },
    { system: '着色器翻译', upstream: 'gfx/tools:Maxwell→SPIR-V→MSL', hos: '止步 SPIR-V,供 Vulkan 消费', state: '待实施' },
    { system: 'App 壳', upstream: 'dm/ios(Sources/*.m,UIKit)', hos: 'ArkUI + XComponent(本仓库 entry)', state: '已完成' },
    { system: '触控界面', upstream: 'DmTouchOverlay.m + dm_touch_controls.c', hos: 'ArkUI 叠加层 + cpp/input 通路', state: '已完成(演示)' },
    { system: '表面渲染', upstream: 'CAMetalLayer', hos: 'XComponent + EGL/GLES3 与 Vulkan 双后端', state: '已完成(演示)' },
    { system: '音频', upstream: 'CoreAudio', hos: 'OHAudio(待接入)', state: '待实施' },
    { system: '构建管线', upstream: 'setup.sh + Xcode project.yml', hos: 'hvigor + CMake(本仓库)', state: '已完成' },
    { system: '验证框架', upstream: 'dm/verify(Unicorn 回归)', hos: 'tools/test_patch_nso_hos.py 合成 NSO 测试 + 上游复用', state: '骨架就绪' }
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

          // 自检与图形后端切换(进入游戏页建立表面后可切换)
          Column({ space: 8 }) {
            Row({ space: 10 }) {
              Text('宿主全链路自检')
                .fontSize(12)
                .fontColor('#ffd9a0')
                .padding({ left: 12, right: 12, top: 6, bottom: 6 })
                .backgroundColor('rgba(60,40,16,0.6)')
                .borderRadius(8)
                .onClick(() => {
                  this.bootReport = DmNative.hostSelfTest();
                })
              Text('Vulkan 演示')
                .fontSize(12)
                .fontColor('#ffd9a0')
                .padding({ left: 12, right: 12, top: 6, bottom: 6 })
                .backgroundColor('rgba(60,40,16,0.6)')
                .borderRadius(8)
                .onClick(() => {
                  this.bootReport = `useVulkan -> ${DmNative.useVulkan()} (0=成功,需先进入游戏页)`;
                })
              Text('GLES 演示')
                .fontSize(12)
                .fontColor('#ffd9a0')
                .padding({ left: 12, right: 12, top: 6, bottom: 6 })
                .backgroundColor('rgba(60,40,16,0.6)')
                .borderRadius(8)
                .onClick(() => {
                  this.bootReport = `useGles -> ${DmNative.useGles()} (0=成功,需先进入游戏页)`;
                })
            }
            Text(this.bootReport)
              .fontSize(11)
              .fontColor('#9fbf8a')
              .maxLines(2)
              .textOverflow({ overflow: TextOverflow.Ellipsis })
          }
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
