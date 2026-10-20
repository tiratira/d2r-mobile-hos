/*
 * SkillButtons — 右下技能键组:点按施放,按住连发,拖拽瞄准。
 * 对应上游 README"press a skill button to cast, hold to keep casting,
 * and drag to aim at any angle",瞄准向量经 DmNative.castSkill 进入
 * native 输入层(上游 hle_private_aim.c 的职责)。
 */
import { DmNative } from '../common/DmNative';

@Component
struct SkillButton {
  slot: number = 0;
  label: string = '';
  sizeVp: number = 84;
  @State aimDx: number = 0;
  @State aimDy: number = 0;
  @State aiming: boolean = false;
  @State pressing: boolean = false;
  private startX: number = 0;
  private startY: number = 0;
  private readonly tapSlop: number = 14;

  private cast(aimDx: number, aimDy: number, hold: number): void {
    DmNative.castSkill(this.slot, aimDx, aimDy, hold);
  }

  build() {
    Stack() {
      Circle({ width: this.sizeVp, height: this.sizeVp })
        .fill(this.pressing ? 'rgba(255,180,80,0.30)' : 'rgba(210,120,50,0.16)')
        .stroke('rgba(255,190,110,0.75)')
        .strokeWidth(2)
      Text(this.label)
        .fontSize(this.sizeVp > 80 ? 16 : 13)
        .fontColor('#ffd9a0')
        .textAlign(TextAlign.Center)
        .width(this.sizeVp)
      if (this.aiming) {
        // 拖拽瞄准指示:从键心指向手指方向
        Line()
          .startPoint([this.sizeVp / 2, this.sizeVp / 2])
          .endPoint([this.sizeVp / 2 + this.aimDx * 60, this.sizeVp / 2 + this.aimDy * 60])
          .stroke('rgba(255,220,150,0.9)')
          .strokeWidth(3)
      }
    }
    .width(this.sizeVp + 40)
    .height(this.sizeVp + 40)
    .gesture(
      PanGesture({ fingers: 1, distance: 1 })
        .onActionStart((event: GestureEvent) => {
          this.pressing = true;
          if (event.fingerList.length > 0) {
            this.startX = event.fingerList[0].localX;
            this.startY = event.fingerList[0].localY;
          }
          this.cast(0, 0, 1); // 按住开始连发
        })
        .onActionUpdate((event: GestureEvent) => {
          if (event.fingerList.length === 0) {
            return;
          }
          let dx: number = event.fingerList[0].localX - this.startX;
          let dy: number = event.fingerList[0].localY - this.startY;
          const len: number = Math.sqrt(dx * dx + dy * dy);
          if (len > this.tapSlop) {
            this.aiming = true;
            this.aimDx = dx / len;
            this.aimDy = dy / len;
            this.cast(this.aimDx, this.aimDy, 1); // 拖拽换向
          }
        })
        .onActionEnd((event: GestureEvent) => {
          this.pressing = false;
          if (!this.aiming) {
            this.cast(0, 0, 0); // 位移极小:视为点按施放
          }
          this.aiming = false;
          this.aimDx = 0;
          this.aimDy = 0;
        })
        .onActionCancel(() => {
          this.pressing = false;
          this.aiming = false;
          this.aimDx = 0;
          this.aimDy = 0;
        })
    )
  }
}

@Component
export struct SkillButtons {
  @State weaponSet: number = 0; // 0=主武器组 1=副武器组(点按切换,上游"two skill sets")
  build() {
    Column({ space: 6 }) {
      Row({ space: 10 }) {
        SkillButton({ slot: 0, label: this.weaponSet === 0 ? '火弹' : '冰弹', sizeVp: 82 })
        Column({ space: 8 }) {
          SkillButton({ slot: 1, label: this.weaponSet === 0 ? '闪电' : '剧毒', sizeVp: 64 })
          Text(this.weaponSet === 0 ? '切换 II' : '切换 I')
            .fontSize(11)
            .fontColor('#d8b57c')
            .padding(4)
            .borderRadius(8)
            .backgroundColor('rgba(255,255,255,0.06)')
            .onClick(() => {
              this.weaponSet = this.weaponSet === 0 ? 1 : 0;
            })
        }
      }
    }
    .alignItems(HorizontalAlign.End)
  }
}
