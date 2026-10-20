/**
 * libdmhost.so 类型声明 — D2R Mobile HOS 原生宿主桥。
 */
export const getHostInfo: () => string;
export const setFilesDir: (dir: string) => void;
export const guestSelfTest: () => string;
/** 宿主全链路自检:加载内嵌 guest → 注册门 → 穿过 dm_gate 执行入口 */
export const hostSelfTest: () => string;
/** 图形探针:Vulkan 实例/设备 + EGL 后端名 + NVN 表自检 */
export const gfxProbe: () => string;
/** 游戏页表面切到 Vulkan 呈现循环(返回 0 成功;需已进入游戏页) */
export const useVulkan: () => number;
/** 游戏页表面切回 EGL/GLES 演示循环(返回 0 成功) */
export const useGles: () => number;
/** phase: 0=down 1=move 2=up 3=cancel;x/y 为像素坐标 */
export const touchInject: (id: number, x: number, y: number, phase: number) => void;
/** 归一化摇杆向量 [-1,1] */
export const setStick: (dx: number, dy: number) => void;
/** 技能施放:slot 0/1,aim 为归一化瞄准向量,hold=按住连续施放 */
export const castSkill: (slot: number, aimDx: number, aimDy: number, hold: number) => void;
