/**
 * libdmhost.so 类型声明 — D2R Mobile HOS 原生宿主桥。
 */
export const getHostInfo: () => string;
export const setFilesDir: (dir: string) => void;
export const guestSelfTest: () => string;
export const gfxProbe: () => string;
/** phase: 0=down 1=move 2=up 3=cancel;x/y 为像素坐标 */
export const touchInject: (id: number, x: number, y: number, phase: number) => void;
/** 归一化摇杆向量 [-1,1] */
export const setStick: (dx: number, dy: number) => void;
/** 技能施放:slot 0/1,aim 为归一化瞄准向量,hold=按住连续施放 */
export const castSkill: (slot: number, aimDx: number, aimDy: number, hold: number) => void;
