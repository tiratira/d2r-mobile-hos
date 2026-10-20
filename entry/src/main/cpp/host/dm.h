/*
 * dm.h — HOS 宿主公共(对应上游 dm/host/dm.h 的本仓子集)。
 */
#ifndef DM_HOST_DM_H
#define DM_HOST_DM_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <hilog/log.h>

#define DM_LOG_TAG "D2RMobileHOS"
#define DMLOG_I(...) OH_LOG_INFO(LOG_APP, __VA_ARGS__)
#define DMLOG_W(...) OH_LOG_WARN(LOG_APP, __VA_ARGS__)
#define DMLOG_E(...) OH_LOG_ERROR(LOG_APP, __VA_ARGS__)
#define LOG(...) DMLOG_I(__VA_ARGS__)
static inline void die(const char *msg)
{
    DMLOG_E("FATAL: %{public}s", msg);
    abort();
}

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t Result;          /* nn::Result:w0,0 = 成功 */
#define RES_OK 0u
#define RES_FAIL 0x2A2u

/* 每线程环境(hle_core.c) */
void dm_guest_thread_init(void);
uint64_t dm_guest_tls_get(void);
void dm_guest_tls_set(uint64_t v);
uint64_t dm_guest_tlr_get(void);
uint64_t dm_param_get(unsigned key_idx);

/* 门分派(hle_core.c;entry_hos.S 的 dm_gate 调入) */
uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs);
void gate_register_abs(uint64_t ret_abs, int kind, uint32_t arg, uint64_t orig_abs);
uint64_t unimpl_c(unsigned idx, uint64_t *regs);
void dm_thread_returned(void);
void dm_thread_enter(uint64_t entry, uint64_t arg, uint64_t sp);

/* svc 分派(hle_svc.c) */
uint64_t svc_dispatch(uint32_t num, uint64_t *regs, uint64_t orig_addr);

/* 启动序(host_boot.c) */
int dmHostBoot(const char *which, char *report, int report_size);

/* 前端接口(front_hos) */
uint64_t front_game_res(int mode);   /* {u32 宽, u32 高} 打包;mode 0=掌机 1=主机 */

#ifdef __cplusplus
}
#endif

#endif /* DM_HOST_DM_H */
