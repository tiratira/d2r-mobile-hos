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

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      // native 画面(暗黑风演示渐变,等待 NVN->Vulkan 回放器接管)
      XComponent({ id: 'game_surface', type: XComponentType.SURFACE, libraryname: 'dmhost' })
        .width('100%')
        .height('100%')
        .onLoad(() => {
          const filesDir: string = AppStorage.get<string>('filesDir') ?? '';
          if (filesDir !== '') {
            DmNative.setFilesDir(filesDir);
          }
        })

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
