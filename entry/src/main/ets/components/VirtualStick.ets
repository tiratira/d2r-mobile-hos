/*
 * VirtualStick — 左下虚拟摇杆。
 * 对应上游 README"Move: a virtual stick with precise diagonals",
 * 输出归一化向量到 native 输入层(dm_input)。
 */
import { DmNative } from '../common/DmNative';

@Component
export struct VirtualStick {
  private centerX: number = 100;
  private centerY: number = 100;
  private radius: number = 68;
  @State knobX: number = 100;
  @State knobY: number = 100;
  @State active: boolean = false;

  private update(x: number, y: number): void {
    if (x < 0) { x = 0; }
    if (y < 0) { y = 0; }
    let dx: number = x - this.centerX;
    let dy: number = y - this.centerY;
    const len: number = Math.sqrt(dx * dx + dy * dy);
    if (len > this.radius) {
      dx = dx / len * this.radius;
      dy = dy / len * this.radius;
    }
    this.knobX = this.centerX + dx;
    this.knobY = this.centerY + dy;
    DmNative.setStick(dx / this.radius, dy / this.radius);
  }

  private reset(): void {
    this.knobX = this.centerX;
    this.knobY = this.centerY;
    this.active = false;
    DmNative.setStick(0, 0);
  }

  build() {
    Stack() {
      Circle({ width: 158, height: 158 })
        .fill(this.active ? 'rgba(255,200,120,0.10)' : 'rgba(200,170,110,0.05)')
        .stroke('rgba(220,190,130,0.40)')
        .strokeWidth(1.5)
      Circle({ width: 66, height: 66 })
        .fill('rgba(235,205,150,0.25)')
        .stroke('rgba(255,225,170,0.85)')
        .strokeWidth(2)
        .position({ x: this.knobX - 33, y: this.knobY - 33 })
    }
    .width(200)
    .height(200)
    .gesture(
      PanGesture({ fingers: 1, distance: 1 })
        .onActionStart((event: GestureEvent) => {
          this.active = true;
          const f = event.fingerList?.[0];
          if (f !== undefined) {
            this.update(f.localX, f.localY);
          }
        })
        .onActionUpdate((event: GestureEvent) => {
          const f = event.fingerList?.[0];
          if (f !== undefined) {
            this.update(f.localX, f.localY);
          }
        })
        .onActionEnd(() => {
          this.reset();
        })
        .onActionCancel(() => {
          this.reset();
        })
    )
  }
}
