/*
 * Index — 主菜单。
 * 对应上游 iOS App 的主菜单前厅:入口按钮 + 宿主自检信息。
 */
import { router } from '@kit.ArkUI';
import { DmNative } from '../common/DmNative';

interface MenuItem {
  title: string;
  route: string;
}

@Entry
@Component
struct Index {
  @State hostInfo: string = '正在初始化原生宿主…';
  private menu: MenuItem[] = [
    { title: '进入游戏', route: 'pages/Game' },
    { title: '移 植 状 态', route: 'pages/PortingStatus' },
    { title: '设 置', route: 'pages/Settings' }
  ];

  aboutToAppear(): void {
    const filesDir: string = AppStorage.get<string>('filesDir') ?? '';
    if (filesDir !== '') {
      DmNative.setFilesDir(filesDir);
    }
    this.hostInfo = DmNative.hostInfo();
  }

  build() {
    Column() {
      Row({ space: 48 }) {
        // 左:标题
        Column({ space: 6 }) {
          Text('DIABLO II')
            .fontSize(44)
            .fontColor('#c7a468')
            .fontWeight(FontWeight.Bold)
            .fontStyle(FontStyle.Italic)
          Text('RESURRECTED')
            .fontSize(22)
            .fontColor('#8f2c1e')
            .fontWeight(FontWeight.Bold)
            .letterSpacing(6)
          Text('Mobile · HarmonyOS 移植框架')
            .fontSize(13)
            .fontColor('#9a8a6e')
            .margin({ top: 8 })
        }
        .alignItems(HorizontalAlign.Start)

        // 右:菜单
        Column({ space: 14 }) {
          ForEach(this.menu, (item: MenuItem) => {
            Text(item.title)
              .fontSize(17)
              .fontColor('#e8d5a8')
              .width(210)
              .textAlign(TextAlign.Center)
              .padding({ top: 10, bottom: 10 })
              .backgroundColor('rgba(30,20,10,0.72)')
              .borderRadius(6)
              .border({ width: 1, color: 'rgba(199,164,104,0.55)' })
              .onClick(() => {
                router.pushUrl({ url: item.route });
              })
          }, (item: MenuItem) => item.title)
        }
        .alignItems(HorizontalAlign.Center)
      }
      .justifyContent(FlexAlign.Center)
      .layoutWeight(1)

      // 底部:宿主自检与声明
      Column({ space: 4 }) {
        Text(this.hostInfo)
          .fontSize(11)
          .fontColor('#7f7259')
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
        Text('本仓库不含任何游戏内容;游戏文件需自行提供(见 docs/PORTING.md)')
          .fontSize(11)
          .fontColor('#6d6250')
      }
      .alignItems(HorizontalAlign.Center)
      .padding(10)
    }
    .width('100%')
    .height('100%')
    .linearGradient({
      direction: GradientDirection.RightBottom,
      colors: [['#231009', 0.0], ['#120806', 0.5], ['#050302', 1.0]] as [ResourceColor, number][]
    })
  }
}
