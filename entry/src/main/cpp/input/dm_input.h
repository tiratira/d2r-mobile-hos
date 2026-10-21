/*
 * dm_input.h — 输入层:触控事件环 + 虚拟摇杆 + 技能施放。
 *
 * 对应上游 dm/ios/Sources/dm_touch_controls.c(触控状态机)与
 * dm/host/hle_private_aim.c(拖拽瞄准:把屏幕方向换算为游戏世界方向)。
 * ArkUI 触控叠加层产生的合成输入与 XComponent 分发的原始触摸
 * 都汇入这里,由渲染/宿主循环每帧消费。
 */
#ifndef DM_INPUT_H
#define DM_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_INPUT_RING 64
#define DM_SKILL_RING 8

typedef enum {
    DM_TOUCH_DOWN = 0,
    DM_TOUCH_MOVE,
    DM_TOUCH_UP,
    DM_TOUCH_CANCEL
} DmTouchPhase;

typedef struct {
    int32_t pointer_id;
    float x, y;          /* 像素坐标 */
    DmTouchPhase phase;
    uint64_t t_us;
} DmTouchEvent;

typedef struct {
    int32_t slot;        /* 技能槽 0/1(主/副技能组) */
    float aim_dx, aim_dy;/* 归一化瞄准方向,点按施放时为 0 */
    int32_t hold;        /* 按住连续施放 */
} DmSkillEvent;

void dmInputInit(void);

/* 原始触摸(线程安全,多生产者) */
void dmInputPushTouch(int32_t pointer_id, float x, float y, DmTouchPhase phase);
int  dmInputPollTouch(DmTouchEvent *out, int max); /* 取走并清空,返回条数 */

/* 摇杆:归一化向量 [-1,1],回中为 (0,0) */
void dmInputSetStick(float dx, float dy);
void dmInputSetButton(int bit,int pressed);
void dmInputGetStick(float *dx, float *dy);

/* 技能施放事件(点按或拖拽瞄准) */
void dmInputCastSkill(int32_t slot, float aim_dx, float aim_dy, int32_t hold);
int  dmInputPollSkills(DmSkillEvent *out, int max);

#ifdef __cplusplus
}
#endif
#endif /* DM_INPUT_H */
