/*
 * hle_hid.c — nn::hid 手柄/触摸服务 HLE(上游 dm/host/hle_hid.c 的 HOS 最小集)。
 *
 * 状态结构与 nn::hid 一致(上游 host 镜像,0x28 字节):
 *   +0x00 sampling(i64) +0x08 buttons(u64) +0x10 lx +0x14 ly +0x18 rx
 *   +0x1c ry +0x20 attributes(u32,bit0 已连接) +0x24 pad
 * 按键位序 = nn::hid::NpadButton(A0 B1 X2 Y3 StickL4 StickR5 L6 R7 ZL8
 * ZR9 Plus10 Minus11 Left12 Up13 Right14 Down15);轴 ±32767,上为正。
 *
 * 触控层/手柄最终都汇到 dmHidSet();按下沿做一次锁存,保证两次采样之间
 * 按下又抬起的键至少被游戏看到一次(菜单点按不丢)。
 */
#include "host/dm.h"

#include <string.h>

typedef struct {
    int64_t sampling;
    uint64_t buttons;
    int32_t lx, ly, rx, ry;
    uint32_t attributes;
    uint32_t pad;
} NpadState;

#define NPAD_ATTR_CONNECTED 1u
#define NPAD_ID_HANDHELD 0x20u
#define STYLE_FULLKEY 1u
#define STYLE_HANDHELD 2u

typedef struct {
    int64_t sampling;
    int32_t count;
    int32_t pad;
    uint8_t touches[16][0x28];
} TouchScreenStates;

static pthread_mutex_t g_hid_mu = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_buttons;
static uint64_t g_latch;   /* 按下沿锁存:交付一次后清 */
static int32_t g_lx, g_ly, g_rx, g_ry;
static int64_t g_sampling;
static int g_inited;

void dmHidInit(void)
{
    if (g_inited) {
        return;
    }
    g_inited = 1;
    DMLOG_I("hid: InitializeNpad/InitializeTouchScreen OK");
}

void dmHidSet(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry)
{
    pthread_mutex_lock(&g_hid_mu);
    g_latch |= buttons & ~g_buttons;   /* 新按下沿 */
    g_buttons = buttons;
    g_lx = lx;
    g_ly = ly;
    g_rx = rx;
    g_ry = ry;
    pthread_mutex_unlock(&g_hid_mu);
}

/* 接了这个样式的手柄 id:掌机 0x20,主机 0 */
static uint32_t connected_id(void)
{
    return dmOeOpMode() == 0 ? NPAD_ID_HANDHELD : 0u;
}

static uint32_t style_of(uint32_t id)
{
    if (id != connected_id()) {
        return 0;
    }
    return dmOeOpMode() == 0 ? STYLE_HANDHELD : STYLE_FULLKEY;
}

static void npad_fill(NpadState *s, uint32_t id, uint32_t style)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    pthread_mutex_lock(&g_hid_mu);
    s->sampling = ++g_sampling;
    if (!(style_of(id) & style)) {
        pthread_mutex_unlock(&g_hid_mu);
        return;
    }
    s->buttons = g_buttons | g_latch;
    s->lx = g_lx;
    s->ly = g_ly;
    s->rx = g_rx;
    s->ry = g_ry;
    s->attributes = NPAD_ATTR_CONNECTED;
    g_latch = 0;   /* 锁存交付一次即清 */
    pthread_mutex_unlock(&g_hid_mu);
}

/* ---------- 强制接管 ---------- */

HLE_FORCE("_ZN2nn3hid14InitializeNpadEv", "手柄服务:宿主自己维护状态",
          void, h_hid_init_npad, void) { dmHidInit(); }

HLE_FORCE("_ZN2nn3hid21InitializeTouchScreenEv", "触摸屏初始化:ArkUI 触控层接同一状态",
          void, h_hid_init_touch, void) { dmHidInit(); }

HLE_FORCE("_ZN2nn3hid22SetSupportedNpadIdTypeEPKjm", "手柄 id 固定一个,忽略",
          void, h_hid_set_ids, const uint32_t *ids, uint64_t n) { (void)ids; (void)n; }

HLE_FORCE("_ZN2nn3hid24SetSupportedNpadStyleSetENS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE",
          "样式跟运行模式走,忽略", void, h_hid_set_styles, uint32_t styles) { (void)styles; }

HLE_FORCE("_ZN2nn3hid29SetNpadHandheldActivationModeENS0_26NpadHandheldActivationModeE",
          "忽略", void, h_hid_set_handheld_mode, int mode) { (void)mode; }

HLE_FORCE("_ZN2nn3hid15GetNpadStyleSetERKj", "当前样式:掌机 Handheld / 主机 FullKey",
          uint32_t, h_hid_get_style, const uint32_t *id)
{
    return id != NULL ? style_of(*id) : 0;
}

HLE_FORCE("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj",
          "FullKey 状态(触控/手柄状态汇入)", void, h_hid_get_fullkey,
          NpadState *s, const uint32_t *id)
{
    npad_fill(s, id != NULL ? *id : 0, STYLE_FULLKEY);
}

HLE_FORCE("_ZN2nn3hid12GetNpadStateEPNS0_17NpadHandheldStateERKj",
          "Handheld 状态(触控/手柄状态汇入)", void, h_hid_get_handheld,
          NpadState *s, const uint32_t *id)
{
    npad_fill(s, id != NULL ? *id : 0, STYLE_HANDHELD);
}

HLE_FORCE("_ZN2nn3hid20IsVibrationPermittedEv", "宿主不震动",
          unsigned, h_hid_vib_perm, void) { return 0; }

HLE_FORCE("_ZN2nn3hid25GetVibrationDeviceHandlesEPNS0_21VibrationDeviceHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE",
          "震动设备数 0(游戏此后不再初始化震动)", int, h_hid_vib_handles,
          void *out, int max, const uint32_t *id, uint32_t style)
{
    (void)out;
    (void)max;
    (void)id;
    (void)style;
    return 0;
}

HLE_FORCE("_ZN2nn3hid25InitializeVibrationDeviceERKNS0_21VibrationDeviceHandleE",
          "空操作", void, h_hid_vib_init, const void *h) { (void)h; }

HLE_FORCE("_ZN2nn3hid18SendVibrationValueERKNS0_21VibrationDeviceHandleERKNS0_14VibrationValueE",
          "空操作", void, h_hid_vib_send, const void *h, const void *v)
{
    (void)h;
    (void)v;
}

HLE_FORCE("_ZN2nn3hid20GetTouchScreenStatesILm16EEEiPNS0_16TouchScreenStateIXT_EEEi",
          "触摸状态:ArkUI 触控层经 dmHidSet 注入,这里给一份空状态",
          int, h_hid_touch_states, TouchScreenStates *out, int n)
{
    if (out == NULL || n < 1) {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->sampling = ++g_sampling;
    out->count = 0;
    return 1;
}

int dmHidSelfTest(char *report, int report_size)
{
    dmHidInit();
    dmHidSet(1ull << 0, 1234, -1234, 0, 0);   /* A + 左摇杆 */
    NpadState s;
    npad_fill(&s, connected_id(), dmOeOpMode() == 0 ? STYLE_HANDHELD : STYLE_FULLKEY);
    int ok = (s.buttons & 1ull) && s.attributes == NPAD_ATTR_CONNECTED &&
             s.lx == 1234 && s.sampling > 0;
    dmHidSet(0, 0, 0, 0, 0);
    snprintf(report, report_size,
             "hid: id=0x%x style=%u buttons=0x%llx ax=(%d,%d) attr=0x%x -> %s",
             connected_id(), style_of(connected_id()),
             (unsigned long long)s.buttons, s.lx, s.ly, s.attributes, ok ? "OK" : "FAIL");
    return ok ? 0 : -1;
}
