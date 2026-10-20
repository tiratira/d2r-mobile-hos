/*
 * BeltBar — 底部腰带:佣兵头像、药水格、菜单/背包入口。
 * 对应上游 README"In camp: belt with potions, mercenary portrait
 * and minimap"的触控版式(演示态:药水可点击消耗)。
 */
import { promptAction } from '@kit.ArkUI';

interface Potion {
  kind: string;
  count: number;
  color: string;
}

@Component
export struct BeltBar {
  @State potions: Potion[] = [
    { kind: '红', count: 4, color: '#b8322a' },
    { kind: '红', count: 3, color: '#b8322a' },
    { kind: '蓝', count: 4, color: '#2e4f9e' },
    { kind: '紫', count: 1, color: '#7a3f9d' }
  ];

  private drink(index: number): void {
    if (this.potions[index].count <= 0) {
      promptAction.showToast({ message: '药水已用完' });
      return;
    }
    const next: Potion[] = [];
    for (let i = 0; i < this.potions.length; i++) {
      const p: Potion = {
        kind: this.potions[i].kind,
        count: i === index ? this.potions[i].count - 1 : this.potions[i].count,
        color: this.potions[i].color
      };
      next.push(p);
    }
    this.potions = next;
    promptAction.showToast({ message: `喝下${this.potions[index].kind}药水` });
  }

  build() {
    Row({ space: 10 }) {
      // 佣兵头像
      Stack() {
        Circle({ width: 40, height: 40 })
          .fill('#3a3128')
          .stroke('#8a7350')
          .strokeWidth(1.5)
        Text('佣')
          .fontSize(15)
          .fontColor('#d8c39a')
      }

      ForEach(this.potions, (p: Potion, index: number) => {
        Stack({ alignContent: Alignment.Bottom }) {
          Circle({ width: 34, height: 34 })
            .fill(p.color)
            .opacity(p.count > 0 ? 0.9 : 0.25)
            .stroke('rgba(255,225,170,0.5)')
            .strokeWidth(1)
          Text(`${p.count}`)
            .fontSize(10)
            .fontColor('#ffe9c4')
            .margin({ bottom: 1 })
        }
        .onClick(() => {
          this.drink(index);
        })
      }, (p: Potion, index: number) => `${p.kind}-${index}-${p.count}`)

      Row({ space: 8 }) {
        Text('背包')
          .fontSize(12)
          .fontColor('#e8d5a8')
          .padding({ left: 10, right: 10, top: 6, bottom: 6 })
          .backgroundColor('rgba(255,255,255,0.07)')
          .borderRadius(8)
          .onClick(() => {
            promptAction.showToast({ message: '背包面板:待 guest 接入(上游 dm_pages.c)' });
          })
        Text('菜单')
          .fontSize(12)
          .fontColor('#e8d5a8')
          .padding({ left: 10, right: 10, top: 6, bottom: 6 })
          .backgroundColor('rgba(255,255,255,0.07)')
          .borderRadius(8)
          .onClick(() => {
            promptAction.showToast({ message: '菜单面板:待 guest 接入' });
          })
      }
    }
    .padding({ left: 12, right: 12, top: 6, bottom: 6 })
    .backgroundColor('rgba(10,7,4,0.55)')
    .borderRadius(12)
    .border({ width: 1, color: 'rgba(200,170,110,0.25)' })
  }
}
