/*
 * dm_platform.h — D2R Mobile HOS 平台抽象层
 *
 * 对应上游 d2r-mobile 的 dm/host 平台层(thread.c / hle_svc.c 中的
 * "thread slot"与 19.2 MHz 系统计数器概念),在 OpenHarmony 上以
 * pthread_key / clock_gettime 实现。
 *
 * 上游背景:重写器把游戏对 x18 / tpdir_el0 的读写和对 svc /
 * cntpct_el0 的执行改写为经由 host gate 进入宿主;每个游戏线程在
 * 宿主里是一个 pthread,guest_thread_init 为该 pthread 填写线程槽。
 * OHOS 内核同为 ARM64 Linux 系,线程槽机制可原样映射为
 * pthread_key + pthread_setspecific。
 */
#ifndef DM_PLATFORM_H
#define DM_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 上游 Horizon 系统计数器频率(Switch),宿主按该频率应答 cntpct_el0。 */
#define DM_COUNTER_HZ 19200000ull

/* 线程槽编号 — 与重写器约定的一致语义:
 *   TCB   — x18 / TLS 块的替身
 *   TLR   — Horizon 线程局部区域(Thread Local Region)的替身
 *   GATE  — host gate 入口地址(从只读 key page 间接获得)
 */
enum {
    DM_SLOT_TCB = 0,
    DM_SLOT_TLR,
    DM_SLOT_GATE,
    DM_SLOT_COUNT
};

int  dmPlatInit(void);
int  dmPlatThreadSlotInit(void);          /* 新游戏线程调用,分配槽区 */
void *dmPlatThreadSlotGet(int slot);
void  dmPlatThreadSlotSet(int slot, void *value);

uint64_t dmPlatCounterRead(void);         /* 单调时钟换算为 19.2 MHz 计数 */
uint64_t dmPlatCounterHz(void);

void   dmPlatSetFilesDir(const char *dir);/* ArkTS 传入应用沙箱 files 目录 */
const char *dmPlatGetFilesDir(void);

#ifdef __cplusplus
}
#endif
#endif /* DM_PLATFORM_H */
