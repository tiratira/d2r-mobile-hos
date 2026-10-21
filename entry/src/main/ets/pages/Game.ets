import { router } from '@kit.ArkUI';
import { DmNative } from '../common/DmNative';
import { VirtualStick } from '../components/VirtualStick';

@Component
struct PadButton {
  label: string = '';
  bit: number = 0;
  @State pressed: boolean = false;
  build() {
    Text(this.label)
      .fontSize(14)
      .fontColor('#ead7ab')
      .textAlign(TextAlign.Center)
      .width(46)
      .height(40)
      .borderRadius(16)
      .backgroundColor(this.pressed ? '#957444' : 'rgba(20,14,8,0.55)')
      .border({ width: 1, color: '#a68a56' })
      .onTouch((event: TouchEvent) => {
        if (event.type === TouchType.Down) {
          this.pressed = true;
          DmNative.setPadButton(this.bit, 1);
        } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
          this.pressed = false;
          DmNative.setPadButton(this.bit, 0);
        }
      })
  }
}

@Entry
@Component
struct Game {
  @State bootStage: string = '加载中';
  private stageTimer: number = -1;
  private startTimer: number = -1;
  private xcController: XComponentController = new XComponentController();

  aboutToDisappear(): void {
    if (this.stageTimer >= 0) { clearInterval(this.stageTimer); }
    if (this.startTimer >= 0) { clearTimeout(this.startTimer); }
    DmNative.setStick(0, 0);
    for (let bit: number = 0; bit < 16; bit++) { DmNative.setPadButton(bit, 0); }
  }

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      XComponent({ id: 'game_surface', type: XComponentType.SURFACE, libraryname: 'dmxe',
        controller: this.xcController })
        .width('100%')
        .height('100%')
        .onLoad(() => {
          const dir: string = AppStorage.get<string>('filesDir') ?? '';
          if (dir !== '') { DmNative.setFilesDir(dir); }
          try { DmNative.setSurfaceId(this.xcController.getXComponentSurfaceId()); } catch (e) {}
          this.startTimer = setTimeout(() => {
            DmNative.hostStart();
            this.bootStage = DmNative.bootStage();
            this.stageTimer = setInterval(() => { this.bootStage = DmNative.bootStage(); }, 1500);
          }, 2000);
        })
      Row({ space: 8 }) {
        Text('‹ 返回').fontSize(12).fontColor('#ead7ab')
          .padding(8).backgroundColor('rgba(0,0,0,0.5)').borderRadius(10)
          .onClick(() => { router.back(); })
        Text(this.bootStage).fontSize(10).fontColor('#9da68a')
      }.margin(8)
      Row({ space: 8 }) {
        PadButton({ label: 'ZL', bit: 8 })
        PadButton({ label: 'L', bit: 6 })
        PadButton({ label: '−', bit: 11 })
        PadButton({ label: '+', bit: 10 })
        PadButton({ label: 'R', bit: 7 })
        PadButton({ label: 'ZR', bit: 9 })
      }.position({ x: '29%', y: 8 })
      VirtualStick().scale({ x: 0.65, y: 0.65 }).position({ x: 0, y: '45%' })
      Column({ space: 5 }) {
        PadButton({ label: '↑', bit: 13 })
        Row({ space: 5 }) {
          PadButton({ label: '←', bit: 12 })
          PadButton({ label: '↓', bit: 15 })
          PadButton({ label: '→', bit: 14 })
        }
      }.position({ x: '26%', y: '72%' })
      Column({ space: 8 }) {
        Row({ space: 8 }) {
          PadButton({ label: 'X', bit: 2 })
          PadButton({ label: 'Y', bit: 3 })
        }
        Row({ space: 8 }) {
          PadButton({ label: 'B 返回', bit: 1 })
          PadButton({ label: 'A 确认', bit: 0 })
        }
      }.position({ x: '81%', y: '61%' })
    }.width('100%').height('100%').backgroundColor('#000000')
  }
}
