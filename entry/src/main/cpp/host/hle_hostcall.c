/*
 * hle_hostcall.c — 0x80 起的宿主调用(剪口约定,上游 hle_tact.c 的 svc_host_call)。
 *
 * 重写器 CUTS 表把 main 的 9 处入口/调用点换成 `svc #0x8n; ret`,门表里
 * kind=svc、arg=0x8n。本仓移植语义(缺省 = 原版行为):
 *   0x80  TACT BLTE 块 zlib 解码 —— 入口剪口,原函数体已不可用,必须真实现;
 *         用系统 libz 按上游逆向的 DecodeZlib 语义(out/out_cap/in/in_len/flags,
 *         override 非空先走 vt[2],flags bit0 校验并跳过 'Z',inflate 整段流,
 *         错误码 Z_BUF_ERROR→4、Z_MEM/DATA/STREAM→1、Z_ERRNO→3)
 *   0x81/0x82  客户端命令/系统消息投递(调用点剪口)—— 转调原目标
 *   0x83/0x84  Granny 段解码(调用点剪口)—— 转调原 0x11f4af0
 *   0x87  地上物品外观路径 —— 转调原 0xa14dc4
 *   0x88  物品落地音 —— 转调原 0x470c20
 *   0x8a/0x8b  进局出生点/尸体落点 —— 转调原 0x5596c0
 * 转调 = 以寄存器原值调原函数、结果回 x0(上游「缺省原样转调」);各调号的
 * 特性增强(掉落光/秘境改道等)留到特性平移阶段。
 *
 * 上游调用号登记表见 dm/host/hle.h;main 模块未加载(x86_64 模拟器)时
 * 转调不可达,记一次警告返回 0。
 */
#include "host/dm.h"

#include <errno.h>
#include <zlib.h>

/* 94012 main.nso 里的原函数偏移(与上游 CUTS 表同一份;转调目标) */
#define MAIN_C2S_DELIVER_OFF 0x632510
#define MAIN_C2S_SYS_OFF     0x6310b0
#define MAIN_GRANNY_DEC_OFF  0x11f4af0
#define MAIN_GROUND_ASSET_OFF 0xa14dc4
#define MAIN_DROP_SOUND_OFF 0x470c20
#define MAIN_JOIN_SPAWN_OFF 0x5596c0

static uint8_t g_seen[16];

/* 客体函数转调:guest 与宿主同地址空间、同 AAPCS64,直接按 8 参调 */
typedef uint64_t (*guest_fn8)(uint64_t, uint64_t, uint64_t, uint64_t,
                              uint64_t, uint64_t, uint64_t, uint64_t);

static uint64_t fwd_main_off(uint64_t off, uint64_t *regs, uint32_t num)
{
    const dm_module_t *m = dm_module_find("main");
    if (!m) {
        if (!g_seen[num - 0x80]++) {
            LOG("host svc %{public}#x:main 未加载,转调不可达(仅 ARM64 真机)", num);
        }
        return 0;
    }
    guest_fn8 fn = (guest_fn8)(uintptr_t)((uint64_t)(uintptr_t)m->base + off);
    return fn(regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
}

/* DecodeZlib(u8 *out, u32 out_cap, const u8 *in, u32 in_len, u32 flags,
 *           u32 stream, Obj *override) → status(0 = 成功) */
static uint64_t tact_decode_zlib(uint64_t *regs)
{
    uint8_t *out = (uint8_t *)(uintptr_t)regs[0];
    uint32_t out_cap = (uint32_t)regs[1];
    const uint8_t *in = (const uint8_t *)(uintptr_t)regs[2];
    uint32_t in_len = (uint32_t)regs[3];
    uint32_t flags = (uint32_t)regs[4];

    if (regs[6]) {   /* override 非空:先调 override->vt[2](...,1),返回假即成功返回 0 */
        uint64_t vtable = 0, vt2 = 0;
        memcpy(&vtable, (void *)(uintptr_t)regs[6], 8);
        if (vtable) memcpy(&vt2, (void *)(uintptr_t)(vtable + 16), 8);
        if (vt2) {
            guest_fn8 fn = (guest_fn8)(uintptr_t)vt2;
            uint64_t ok = fn(regs[6], regs[0], regs[1], regs[2], regs[3], 1, 0, 0);
            if (!(ok & 0xff)) {
                return 0;
            }
        }
    }
    if (flags & 1) {
        if (in_len < 2 || in[0] != 'Z') {
            return 8;
        }
        in++;
        in_len--;
    }
    z_stream zs;
    memset(&zs, 0, sizeof zs);
    if (inflateInit(&zs) != Z_OK) {
        return 1;
    }
    zs.next_in = (Bytef *)(uintptr_t)in;
    zs.avail_in = in_len;
    zs.next_out = out;
    zs.avail_out = out_cap;
    int rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (rc == Z_STREAM_END) {
        return 0;   /* 到达流尾即成功;输出可短于容量 */
    }
    switch (rc) {
    case Z_BUF_ERROR: return 4;
    case Z_MEM_ERROR: case Z_DATA_ERROR: case Z_STREAM_ERROR: return 1;
    case Z_ERRNO: return 3;
    default: return 1;
    }
}

uint64_t svc_host_call(uint32_t num, uint64_t *regs, uint64_t orig)
{
    (void)orig;
    switch (num) {
    case 0x80:
        regs[0] = tact_decode_zlib(regs);
        return 1;
    case 0x81:
        regs[0] = fwd_main_off(MAIN_C2S_DELIVER_OFF, regs, num);
        return 1;
    case 0x82:
        regs[0] = fwd_main_off(MAIN_C2S_SYS_OFF, regs, num);
        return 1;
    case 0x83:
    case 0x84:
        regs[0] = fwd_main_off(MAIN_GRANNY_DEC_OFF, regs, num);
        return 1;
    case 0x87:
        regs[0] = fwd_main_off(MAIN_GROUND_ASSET_OFF, regs, num);
        return 1;
    case 0x88:
        regs[0] = fwd_main_off(MAIN_DROP_SOUND_OFF, regs, num);
        return 1;
    case 0x8a:
    case 0x8b:
        regs[0] = fwd_main_off(MAIN_JOIN_SPAWN_OFF, regs, num);
        return 1;
    default:
        if (num <= 0x8f && !g_seen[num - 0x80]++) {
            LOG("host svc %{public}#x 未接线(占位成功)", num);
        }
        regs[0] = 0;
        return 1;
    }
}
