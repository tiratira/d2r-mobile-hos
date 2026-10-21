/*
 * Game — 游戏页。
 * XComponent(libraryname="dmhost")承载 native EGL 渲染循环;
 * 触控叠加层(摇杆/技能/腰带/Boss 血条/小地图)以 ArkUI 悬浮其上,
 * 对应上游 dm/ios/Sources/DmTouchOverlay.m 的角色分工:native 画面
 * 在下,触控层在上,合成输入经 DmNative 注入。
 */
import { router } from '@kit.ArkUI';
import { DmNative } from '../common/DmNative';
import { VirtualStick } from '../components/VirtualStick';
import { SkillButtons } from '../components/SkillButtons';
import { BeltBar } from '../components/BeltBar';
import { BossBar } from '../components/BossBar';

@Entry
@Component
struct Game {
  @State bossHp: number = 76.4;
  @State bossName: string = '都瑞尔 Duriel';
  @State abi: string = '';
  @State bootInfo: string = '';
  @State bootStage: string = '未启动';
  private stageTimer: number = -1;
  private xcController: XComponentController = new XComponentController();

  aboutToAppear(): void {
    this.abi = DmNative.hostAbi();
  }

  aboutToDisappear(): void {
    if (this.stageTimer >= 0) {
      clearInterval(this.stageTimer);
      this.stageTimer = -1;
    }
  }

  /** 进入游戏:真机(ARM64)自动走 SDK 启动序;模拟器只做预检并显示原因 */
  private startGuest(): void {
    this.bootInfo = DmNative.hostStart();
    this.bootStage = DmNative.bootStage();
    if (this.stageTimer < 0) {
      this.stageTimer = setInterval(() => {
        this.bootStage = DmNative.bootStage();
      }, 1500);
    }
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      // native 画面(暗黑风演示渐变,等待 NVN->Vulkan 回放器接管)
      XComponent({ id: 'game_surface', type: XComponentType.SURFACE, libraryname: 'dmxe',
        controller: this.xcController })
        .width('100%')
        .height('100%')
        .onLoad(() => {
          const filesDir: string = AppStorage.get<string>('filesDir') ?? '';
          if (filesDir !== '') {
            DmNative.setFilesDir(filesDir);
          }
          // 原生 surface 回调在本机没有送达宿主,改为用 controller 拿
          // surfaceId 直接建 OHNativeWindow 再起 Vulkan(回调可用时幂等)。
          try {
            const sid: string = this.xcController.getXComponentSurfaceId();
            if (sid !== undefined && sid !== null && sid.length > 0) {
              DmNative.setSurfaceId(sid);
            }
          } catch (e) {
            // surfaceId 尚不可用,等 surface 回调兜底
          }
          setTimeout(() => {
            this.startGuest();
          }, 3000);
        })

      // 运行形态说明:模拟器(x86_64)不执行 ARM64 游戏,避免把演示渐变误认成游戏画面
      if (this.abi !== 'arm64') {
        Column({ space: 4 }) {
          Text('模拟器模式(x86_64)')
            .fontSize(12)
            .fontColor('#e8b46a')
            .fontWeight(FontWeight.Bold)
          Text('本机只跑移植框架层 + 演示渲染,不执行 ARM64 游戏客体。\n游戏画面需要 ARM64 鸿蒙真机(连接后 devecocli run 即可)。')
            .fontSize(10)
            .fontColor('#b7a67e')
            .textAlign(TextAlign.Center)
        }
        .position({ x: '20%', y: '42%' })
        .width('60%')
        .padding(10)
        .backgroundColor('rgba(20,12,6,0.72)')
        .borderRadius(10)
        .border({ width: 1, color: 'rgba(200,170,110,0.25)' })
      }

      // 顶部中央:Boss 血条
      Column() {
        BossBar({ hp: this.bossHp, bossName: this.bossName })
      }
      .width('100%')
      .alignItems(HorizontalAlign.Center)
      .padding({ top: 6 })

      // 左上:退出
      Text('‹ 退出')
        .fontSize(13)
        .fontColor('rgba(230,210,170,0.8)')
        .padding({ left: 10, right: 10, top: 5, bottom: 5 })
        .backgroundColor('rgba(0,0,0,0.35)')
        .borderRadius(10)
        .margin({ left: 12, top: 10 })
        .onClick(() => {
          router.back();
        })

      // 启动状态(真机显示 BOOT 阶段;模拟器显示预检摘要)
      Column({ space: 2 }) {
        Text(this.abi === 'arm64' ? `启动阶段:${this.bootStage}` : '框架层已加载 · 游戏未执行')
          .fontSize(10)
          .fontColor(this.abi === 'arm64' ? '#9fbf8a' : '#bf9a6a')
        if (this.bootInfo !== '') {
          Text(this.bootInfo)
            .fontSize(8)
            .fontColor('#8a7c62')
            .maxLines(3)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
      }
      .position({ x: 12, y: 52 })
      .width('64%')
      .alignItems(HorizontalAlign.Start)

      // 右上:小地图占位
      Column() {
        Text('地图')
          .fontSize(11)
          .fontColor('#b7a67e')
      }
      .justifyContent(FlexAlign.Center)
      .width(84)
      .height(84)
      .backgroundColor('rgba(10,7,4,0.5)')
      .borderRadius(10)
      .border({ width: 1, color: 'rgba(200,170,110,0.35)' })
      .position({ x: '88%', y: 12 })

      // 左下:虚拟摇杆
      VirtualStick()
        .position({ x: 20, y: '68%' })

      // 右下:技能键组
      SkillButtons()
        .position({ x: '76%', y: '58%' })

      // 底部中央:腰带
      Column() {
        BeltBar()
      }
      .width('100%')
      .alignItems(HorizontalAlign.Center)
      .padding({ bottom: 8 })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#000000')
  }
}
