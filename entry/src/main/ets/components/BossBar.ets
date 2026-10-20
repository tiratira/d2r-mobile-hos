/*
 * BossBar — 顶部 Boss 血条。
 * 对应上游 README"Boss health bar: pinned to the top of the screen
 * with the percentage remaining"。
 */
@Component
export struct BossBar {
  @Prop hp: number = 100;
  @Prop bossName: string = '';

  build() {
    Column({ space: 3 }) {
      Row({ space: 8 }) {
        Text(this.bossName)
          .fontSize(13)
          .fontColor('#f0dcae')
          .fontWeight(FontWeight.Bold)
        Text(`${this.hp.toFixed(1)}%`)
          .fontSize(12)
          .fontColor('#e8a0a0')
      }
      Progress({ value: this.hp, total: 100, type: ProgressType.Linear })
        .width(320)
        .height(8)
        .color('#a52a1e')
        .backgroundColor('rgba(60,10,8,0.8)')
        .borderRadius(4)
        .style({ strokeWidth: 8 })
    }
    .padding({ left: 14, right: 14, top: 4, bottom: 6 })
    .backgroundColor('rgba(8,5,3,0.55)')
    .borderRadius(10)
    .border({ width: 1, color: 'rgba(200,170,110,0.25)' })
  }
}
