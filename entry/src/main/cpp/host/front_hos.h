/*
 * front_hos.h/.c — 前端接口的 HOS 实现(上游 host/front.h 的本仓子集)。
 */
#ifndef DM_HOST_FRONT_HOS_H
#define DM_HOST_FRONT_HOS_H

#include <stdint.h>

/* 输出分辨率 {u32 宽, u32 高} 打包;mode 0=掌机 1=主机。
 * 真实接入后取渲染表面尺寸;占位阶段给上游的 Mac 缺省值。 */
uint64_t front_game_res(int mode);

/* 前端覆盖分辨率(游戏页加载表面尺寸后调用)。 */
void front_set_game_res(int mode, uint32_t w, uint32_t h);

#endif
