/*
 * Settings — 设置页(演示态,值存 AppStorage)。
 * 对应上游游戏内设置的触控相关项:灵敏度、亮度增强、帧率、语言。
 */
import { router } from '@kit.ArkUI';

@Entry
@Component
struct Settings {
  @State sensitivity: number = Number(AppStorage.get<number>('sensitivity') ?? 50);
  @State brightBoost: number = Number(AppStorage.get<number>('brightBoost') ?? 0);
  @State fps30: boolean = (AppStorage.get<number>('fpsCap') ?? 60) === 30;
  @State langIndex: number = 0;
  private langOptions: SelectOption[] = [
    { value: '简体中文' },
    { value: 'English' },
    { value: 'Español' },
    { value: 'Русский' }
  ];

  build() {
    Column({ space: 18 }) {
      Text('设 置')
        .fontSize(24)
        .fontColor('#c7a468')
        .fontWeight(FontWeight.Bold)
        .margin({ top: 24 })

      Column({ space: 6 }) {
        Text(`触控灵敏度:${this.sensitivity}`)
          .fontSize(14)
          .fontColor('#e8d5a8')
          .width('80%')
        Slider({ value: this.sensitivity, min: 10, max: 100, step: 5 })
          .width('80%')
          .selectedColor('#a5793c')
          .onChange((value: number) => {
            this.sensitivity = value;
            AppStorage.setOrCreate('sensitivity', value);
          })
      }
      .width('100%')
      .alignItems(HorizontalAlign.Start)
      .padding({ left: '10%' })

      Column({ space: 6 }) {
        Text(`3D 画面增亮:+${this.brightBoost}%`)
          .fontSize(14)
          .fontColor('#e8d5a8')
          .width('80%')
        Slider({ value: this.brightBoost, min: 0, max: 40, step: 5 })
          .width('80%')
          .selectedColor('#a5793c')
          .onChange((value: number) => {
            this.brightBoost = value;
            AppStorage.setOrCreate('brightBoost', value);
          })
      }
      .width('100%')
      .alignItems(HorizontalAlign.Start)
      .padding({ left: '10%' })

      Row({ space: 20 }) {
        Text('帧率上限')
          .fontSize(14)
          .fontColor('#e8d5a8')
        Radio({ value: '60', group: 'fps' })
          .checked(!this.fps30)
          .onChange((checked: boolean) => {
            if (checked) {
              this.fps30 = false;
              AppStorage.setOrCreate('fpsCap', 60);
            }
          })
        Text('60')
          .fontSize(13)
          .fontColor('#9a8a6e')
        Radio({ value: '30', group: 'fps' })
          .checked(this.fps30)
          .onChange((checked: boolean) => {
            if (checked) {
              this.fps30 = true;
              AppStorage.setOrCreate('fpsCap', 30);
            }
          })
        Text('30(省电)')
          .fontSize(13)
          .fontColor('#9a8a6e')
      }

      Column({ space: 6 }) {
        Text('界面语言(对应上游 i18n/ui_text.tsv)')
          .fontSize(14)
          .fontColor('#e8d5a8')
        Select(this.langOptions)
          .selected(this.langIndex)
          .value(this.langOptions[this.langIndex].value)
          .fontColor('#e8d5a8')
          .selectedOptionFontColor('#c7a468')
          .onSelect((index: number) => {
            this.langIndex = index;
          })
      }
      .width('100%')
      .alignItems(HorizontalAlign.Start)
      .padding({ left: '10%' })

      Text('‹ 返回')
        .fontSize(14)
        .fontColor('#e8d5a8')
        .padding({ left: 14, right: 14, top: 6, bottom: 6 })
        .backgroundColor('rgba(30,20,10,0.72)')
        .borderRadius(8)
        .margin({ top: 12 })
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
