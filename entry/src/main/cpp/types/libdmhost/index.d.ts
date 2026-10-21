/**
 * libdmhost.so 类型声明 — D2R Mobile HOS 原生宿主桥。
 */
export const getHostInfo: () => string;
/** 实际 ABI:"arm64"(真机)| "x86_64"(PC 模拟器,data-only) */
export const hostAbi: () => string;
/** 1 = 能执行游戏客体(ARM64);0 = 仅框架层(x86_64 模拟器) */
export const guestExecSupported: () => number;
export const setFilesDir: (dir: string) => void;
export const guestSelfTest: () => string;
/** 宿主全链路自检:加载内嵌 guest → 注册门 → 穿过 dm_gate 执行入口 */
export const hostSelfTest: () => string;
/** 真实 SDK 启动序:x86 只做预检,ARM64 真机起客体主线程跑 _init_libc0→…→nnMain */
export const hostStart: () => string;
/** 启动阶段字符串(最后到达的 _init_libc*/nninit*/nnMain 阶段) */
export const bootStage: () => string;
/** fs/oe/hid/time 服务层 HLE 自检 */
export const serviceSelfTest: () => string;
/** OHAudio 渲染器自检(48kHz/2ch/S16LE) */
export const audioSelfTest: () => string;
/** 播 800ms 440Hz 正弦,验证音频通路 */
export const playTone: () => string;
/** 图形探针:Vulkan 实例/设备 + EGL 后端名 + NVN 表自检 */
export const gfxProbe: () => string;
/** 游戏页表面切到 Vulkan 呈现循环(返回 0 成功;需已进入游戏页) */
export const useVulkan: () => number;
/** 用 XComponent 的 surfaceId 建 OHNativeWindow 并起 Vulkan(回调未送达时兜底) */
export const setSurfaceId: (surfaceId: string) => number;
/** 游戏页表面切回 EGL/GLES 演示循环(返回 0 成功) */
export const useGles: () => number;
/** phase: 0=down 1=move 2=up 3=cancel;x/y 为像素坐标 */
export const touchInject: (id: number, x: number, y: number, phase: number) => void;
/** 归一化摇杆向量 [-1,1] */
export const setStick: (dx: number, dy: number) => void;
/** 技能施放:slot 0/1,aim 为归一化瞄准向量,hold=按住连续施放 */
export const castSkill: (slot: number, aimDx: number, aimDy: number, hold: number) => void;

export const setPadButton: (bit: number, pressed: number) => void;
