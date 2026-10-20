/*
 * hle_oe.c — nn::oe 运行环境服务 HLE(上游 dm/host/hle_oe.c 的 HOS 最小集)。
 *
 * Horizon 上 am/oe 服务负责运行模式(掌机/主机)、性能模式、焦点与
 * 系统通知队列。宿主按鸿蒙设备自身状态应答:
 *   运行模式:手机=掌机(0);可用 dmOeSetOpMode 由前端切
 *   焦点:1 前台 / 2 失焦 / 3 后台,前端生命周期调 dmOeSetFocus
 *   通知:32 槽环形队列,消息号 15 焦点 / 30 运行模式 / 31 性能模式
 *        (游戏 main 0xe10d30 只处理这三个;同类未取走时合并)
 *
 * 注意 bool 形参按上游约定声明成 unsigned 再截断(Switch 不扩展 <32 位实参)。
 */
#include "host/dm.h"

#include <pthread.h>
#include <stdlib.h>

static int g_opmode;          /* 0 掌机 / 1 主机 */
static int g_focus = 1;       /* 1 前台 */
static int g_ntf_opmode = 1;  /* 默认开:游戏启动后要收 30 */
static int g_ntf_perfmode = 1;
static int g_ntf_focus = 1;

#define OE_NTF_MAX 32
static uint32_t g_ntf[OE_NTF_MAX];
static int g_ntf_head, g_ntf_tail;
static pthread_mutex_t g_ntf_mu = PTHREAD_MUTEX_INITIALIZER;

void dmOeInit(void)
{
    static int done;
    if (done) {
        return;
    }
    done = 1;
    const char *m = getenv("DM_OPMODE");
    g_opmode = m != NULL ? atoi(m) : 0;
    DMLOG_I("oe: 运行模式=%{public}d(0 掌机/1 主机)", g_opmode);
}

/* 同类型未取走则合并(这些是"查询当前状态"的失效通知,不是事件流) */
static void ntf_push(uint32_t msg)
{
    pthread_mutex_lock(&g_ntf_mu);
    for (int i = g_ntf_head; i != g_ntf_tail; i = (i + 1) % OE_NTF_MAX) {
        if (g_ntf[i] == msg) {
            pthread_mutex_unlock(&g_ntf_mu);
            return;
        }
    }
    int next = (g_ntf_tail + 1) % OE_NTF_MAX;
    if (next != g_ntf_head) {
        g_ntf[g_ntf_tail] = msg;
        g_ntf_tail = next;
    }
    pthread_mutex_unlock(&g_ntf_mu);
}

static int ntf_pop(uint32_t *out)
{
    pthread_mutex_lock(&g_ntf_mu);
    if (g_ntf_head == g_ntf_tail) {
        pthread_mutex_unlock(&g_ntf_mu);
        return 0;
    }
    *out = g_ntf[g_ntf_head];
    g_ntf_head = (g_ntf_head + 1) % OE_NTF_MAX;
    pthread_mutex_unlock(&g_ntf_mu);
    return 1;
}

void dmOeSetOpMode(int docked)
{
    int v = docked ? 1 : 0;
    if (v == g_opmode) {
        return;
    }
    g_opmode = v;
    if (g_ntf_opmode) {
        ntf_push(30);
    }
    if (g_ntf_perfmode) {
        ntf_push(31);
    }
}

int dmOeOpMode(void)
{
    return g_opmode;
}

void dmOeSetFocus(int state)
{
    if (state < 1 || state > 3 || state == g_focus) {
        return;
    }
    g_focus = state;
    if (g_ntf_focus) {
        ntf_push(15);
    }
}

/* ---------- 强制接管(Horizon 服务客户端 → 宿主默认值) ---------- */

HLE_FORCE("_ZN2nn2oe15EnableRecordingEv", "屏幕录像宿主不支持,空操作",
          void, h_oe_enable_rec, void) { (void)0; }
HLE_FORCE("_ZN2nn2oe16DisableRecordingEv", "屏幕录像宿主不支持,空操作",
          void, h_oe_disable_rec, void) { (void)0; }
HLE_FORCE("_ZN2nn2oe16EnableScreenShotEv", "截图由系统负责,空操作",
          void, h_oe_enable_shot, void) { (void)0; }
HLE_FORCE("_ZN2nn2oe17DisableScreenShotEv", "截图由系统负责,空操作",
          void, h_oe_disable_shot, void) { (void)0; }

HLE_FORCE("_ZN2nn2oe16GetOperationModeEv", "运行模式跟鸿蒙设备(手机=掌机)",
          int, h_oe_get_opmode, void) { return g_opmode; }

HLE_FORCE("_ZN2nn2oe18GetPerformanceModeEv", "性能模式:掌机 Normal/主机 Boost",
          int, h_oe_get_perfmode, void) { return g_opmode ? 1 : 0; }

HLE_FORCE("_ZN2nn2oe27SetPerformanceConfigurationENS0_15PerformanceModeEi",
          "宿主不调频,只记日志", void, h_oe_set_perfcfg, int mode, int cfg)
{
    (void)mode;
    (void)cfg;
}

HLE_FORCE("_ZN2nn2oe20GetCurrentFocusStateEv", "焦点态由前端生命周期驱动",
          int, h_oe_get_focus, void) { return g_focus; }

HLE_FORCE("_ZN2nn2oe20SetFocusHandlingModeENS0_17FocusHandlingModeE",
          "焦点处理模式只记日志", void, h_oe_set_focus_mode, int mode) { (void)mode; }

HLE_FORCE("_ZN2nn2oe25TryPopNotificationMessageEPj",
          "系统通知队列:15 焦点/30 运行模式/31 性能模式",
          unsigned, h_oe_pop_ntf, uint32_t *msg)
{
    if (msg == NULL) {
        return 0;
    }
    uint32_t m;
    if (!ntf_pop(&m)) {
        return 0;
    }
    *msg = m;
    /* 取走通知时把当前分辨率参数位推到本线程槽:游戏在取通知的线程上
     * 读参数位改出图尺寸,写晚了会用旧尺寸。KRES0=4,KRES1=5。 */
    dm_param_set(4, front_game_res(0));
    dm_param_set(5, front_game_res(1));
    return 1;
}

HLE_FORCE("_ZN2nn2oe28SetResumeNotificationEnabledEb", "恢复通知用焦点 15 表达,空操作",
          void, h_oe_set_resume_ntf, unsigned on) { (void)on; }

HLE_FORCE("_ZN2nn2oe42SetOperationModeChangedNotificationEnabledEb",
          "运行模式变化通知开关", void, h_oe_set_opmode_ntf, unsigned on)
{
    g_ntf_opmode = on & 0xff;
}

HLE_FORCE("_ZN2nn2oe44SetPerformanceModeChangedNotificationEnabledEb",
          "性能模式变化通知开关", void, h_oe_set_perfmode_ntf, unsigned on)
{
    g_ntf_perfmode = on & 0xff;
}

HLE_FORCE("_ZN2nn2oe18GetDesiredLanguageEv",
          "系统语言:返回 nn::settings::LanguageCode(char[8] 按值)",
          uint64_t, h_oe_get_lang, void)
{
    const char *lang = getenv("DM_LANG");
    if (lang == NULL || lang[0] == 0) {
        lang = "zh-Hans";
    }
    uint8_t b[8] = {0};
    for (int i = 0; i < 7 && lang[i]; i++) {
        b[i] = (uint8_t)lang[i];
    }
    uint64_t v = 0;
    memcpy(&v, b, sizeof(v));
    return v;
}

HLE_FORCE("_ZN2nn2oe17GetPseudoDeviceIdEPNS_4util4UuidE",
          "伪设备号:固定 Uuid", void, h_oe_get_pseudo_id, uint8_t *uuid)
{
    static const uint8_t id[16] = {
        0x78, 0x62, 0x6c, 0x00, 0x53, 0x57, 0x4e, 0x41,
        0x54, 0x49, 0x56, 0x45, 0x31, 0x30, 0x30, 0x31,
    };
    if (uuid != NULL) {
        memcpy(uuid, id, sizeof(id));
    }
}

int dmOeSelfTest(char *report, int report_size)
{
    dmOeInit();
    ntf_push(99);
    uint32_t got = 0;
    int ok = ntf_pop(&got) && got == 99;
    DMLOG_I("oe: 自检 mode=%{public}d focus=%{public}d 队列=%{public}s",
            g_opmode, g_focus, ok ? "OK" : "FAIL");
    snprintf(report, report_size,
             "oe: 模式=%d 焦点=%d 通知队列=%s | 语言/设备号/性能模式已接管",
             g_opmode, g_focus, ok ? "OK" : "FAIL");
    return ok ? 0 : -1;
}
