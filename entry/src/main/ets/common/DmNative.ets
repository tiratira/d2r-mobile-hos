/*
 * DmNative — libdmhost.so 的 ArkTS 封装。
 * 上游对应 dm/ios/Sources/dm_touch_game_bridge.c(ArkUI 触控层与
 * native 宿主之间的桥)。
 */
import dmhost from 'libdmhost.so';

export class DmNative {
  static hostInfo(): string {
    return dmhost.getHostInfo();
  }

  static setFilesDir(dir: string): void {
    dmhost.setFilesDir(dir);
  }

  static guestSelfTest(): string {
    return dmhost.guestSelfTest();
  }

  static hostSelfTest(): string {
    return dmhost.hostSelfTest();
  }

  static gfxProbe(): string {
    return dmhost.gfxProbe();
  }

  static useVulkan(): number {
    return dmhost.useVulkan();
  }

  static useGles(): number {
    return dmhost.useGles();
  }

  /** phase: 0=down 1=move 2=up 3=cancel;x/y 传像素坐标(vp2px 换算后) */
  static touchInject(id: number, x: number, y: number, phase: number): void {
    dmhost.touchInject(id, x, y, phase);
  }

  static setStick(dx: number, dy: number): void {
    dmhost.setStick(dx, dy);
  }

  static castSkill(slot: number, aimDx: number, aimDy: number, hold: number): void {
    dmhost.castSkill(slot, aimDx, aimDy, hold);
  }
}
