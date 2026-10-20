/*
 * hle_misc.c — aoc / err / ngc / nsd / pctl / ec 等杂项服务 HLE
 * (上游 dm/host/hle_misc.c 的 HOS 最小集)。
 *
 * 原则:会写进游戏内存的接口必须真写(列表/掩码),纯 Result 的接口可失败。
 *   aoc  : 已拥有追加内容 = 1(术士君临),Count/List 真写
 *   err  : 只把错误号打到 hilog,不弹窗
 *   ngc  : 无词库,CheckProfanityWords 必须把 masks 全写 0
 *   nsd  : 无联网服务,Resolve 失败
 *   pctl : 单机无家长控制,自由通信不允许
 */
#include "host/dm.h"

#include <stdlib.h>

/* ---------- aoc(追加内容) ---------- */
HLE_FORCE("_ZN2nn3aoc17CountAddOnContentEv", "已拥有 1 个追加内容(术士君临);DM_AOC_OWNED=0 可关",
          int, h_aoc_count, void)
{
    const char *e = getenv("DM_AOC_OWNED");
    return (e != NULL && atoi(e) == 0) ? 0 : 1;
}

HLE_FORCE("_ZN2nn3aoc16ListAddOnContentEPiii", "按下标写已拥有追加内容名(1)",
          int, h_aoc_list, int *out, int off, int n)
{
    int cnt = h_aoc_count();
    if (out == NULL || off < 0 || n <= 0) {
        return 0;
    }
    int written = 0;
    for (int i = 0; i < n; i++) {
        int idx = off + i;
        if (idx >= cnt) {
            break;
        }
        out[i] = 1;
        written++;
    }
    return written;
}

HLE_FORCE("_ZN2nn3aoc31GetAddOnContentListChangedEventEPNS_2os11SystemEventE",
          "宿主不产生 DLC 列表变化事件", void, h_aoc_event, void *ev) { (void)ev; }

HLE_FORCE("_ZN2nn2ec24ShowShopAddOnContentListEv", "商店属排除范围,空操作",
          void, h_ec_shop, void) { (void)0; }

/* ---------- err ---------- */
HLE_FORCE("_ZN2nn3err9ShowErrorENS_6ResultE", "错误只打 hilog,不弹窗",
          void, h_err_show, Result r)
{
    DMLOG_E("nn::err::ShowError module=%{public}u desc=%{public}u",
            r & 0x1ffu, (r >> 9) & 0x1fffu);
}

/* ---------- ngc(脏话过滤):无词库 ---------- */
HLE_FORCE("_ZN2nn3ngc15ProfanityFilterC1Ev", "不构造(宿主无词库对象)", void, h_ngc_ctor, void *self)
{
    (void)self;
}

HLE_FORCE("_ZN2nn3ngc15ProfanityFilterD1Ev", "空", void, h_ngc_dtor, void *self) { (void)self; }

HLE_FORCE("_ZN2nn3ngc15ProfanityFilter10InitializeEPvm", "无词库,直接成功",
          Result, h_ngc_init, void *self, void *work, uint64_t sz)
{
    (void)self;
    (void)work;
    (void)sz;
    return RES_OK;
}

HLE_FORCE("_ZN2nn3ngc15ProfanityFilter17GetContentVersionEv", "无词库,版本 0",
          uint32_t, h_ngc_version, void *self) { (void)self; return 0; }

HLE_FORCE("_ZN2nn3ngc15ProfanityFilter19CheckProfanityWordsEPjjPPKcm",
          "把 masks 全写 0(无脏话)", Result, h_ngc_check, void *self,
          uint32_t *masks, uint32_t pat, const char **words, uint64_t n)
{
    (void)self;
    (void)pat;
    (void)words;
    if (masks != NULL) {
        for (uint64_t i = 0; i < n; i++) {
            masks[i] = 0;
        }
    }
    return RES_OK;
}

/* ---------- nsd(网络服务发现) ---------- */
HLE_FORCE("_ZN2nn3nsd7ResolveEPNS0_4FqdnERKS1_", "无联网服务,解析失败",
          Result, h_nsd_resolve, void *out, const void *in)
{
    (void)out;
    (void)in;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn3nsd9ResolveExEPNS0_4FqdnERKS1_", "无联网服务,解析失败",
          Result, h_nsd_resolve_ex, void *out, const void *in)
{
    (void)out;
    (void)in;
    return RES_FAIL;
}

/* ---------- pctl(家长控制) ---------- */
HLE_FORCE("_ZN2nn4pctl25TryBeginFreeCommunicationEb", "单机无家长控制:不允许自由通信",
          unsigned, h_pctl_begin_free, unsigned show_ui)
{
    (void)show_ui;
    return 0;
}

HLE_FORCE("_ZN2nn4pctl20EndFreeCommunicationEv", "空操作",
          void, h_pctl_end_free, void) { (void)0; }

int dmMiscSelfTest(char *report, int report_size)
{
    int owned = h_aoc_count();
    int list[2] = {-1, -1};
    int n = h_aoc_list(list, 0, 2);
    uint32_t masks[3] = {0xdeadbeefu, 0xdeadbeefu, 0xdeadbeefu};
    int ok = (n == owned) && (owned == 0 || list[0] == 1) && h_ngc_check(NULL, masks, 0, NULL, 3) == RES_OK;
    for (int i = 0; i < 3; i++) {
        ok = ok && masks[i] == 0;
    }
    snprintf(report, report_size, "misc: aoc=%d/ex%02x | profanity=%s",
             owned, list[0], ok ? "OK" : "FAIL");
    return ok ? 0 : -1;
}
