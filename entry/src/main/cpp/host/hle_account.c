/*
 * hle_account.c — nn::account 本地用户模型(上游 dm/host/hle_account.c 的 HOS 最小集)。
 *
 * 单机宿主只有一个本地用户:
 *   Uid        = {0x78626c00_00000001, 0x53574e41_54495645}(16 字节,非零即有效)
 *   UserHandle = {uid.v0, uid.v1, 0x55534552484e444c("USERHNDL" 魔数)}(24 字节)
 * 昵称缺省 "Player",DM_NICK 可覆盖(最长 32 字节)。
 * 联网类接口(NetworkServiceAccount / 任天堂账号授权)一律失败 —— 与上游一致。
 *
 * 注意:UserHandle 的魔数不匹配时上游 die;这里返回 RES_FAIL 并打日志,
 * 避免真机上一条异常数据把整个进程带走。
 */
#include "host/dm.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t v[2];
} Uid;

typedef struct {
    uint64_t v[3];
} UserHandle;

static const Uid k_uid = {{0x78626c0000000001ull, 0x53574e4154495645ull}};
#define USER_HANDLE_MAGIC 0x55534552484e444cull   /* "USERHNDL" */

static int handle_valid(const UserHandle *h)
{
    return h != NULL && h->v[2] == USER_HANDLE_MAGIC;
}

HLE_FORCE("_ZN2nn7account10InitializeEv", "本地用户模型,不连 PCF/BAAS",
          void, h_acc_init, void) { (void)0; }

HLE_FORCE("_ZN2nn7account13ListOpenUsersEPiPNS0_3UidEi",
          "列出打开的用户:1 个本地用户", Result, h_acc_list_users,
          int *n, Uid *out, int max)
{
    if (n != NULL) {
        *n = (max > 0 && out != NULL) ? 1 : 0;
    }
    if (max > 0 && out != NULL) {
        out[0] = k_uid;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn7account16ShowUserSelectorEPNS0_3UidERKNS0_21UserSelectionSettingsE",
          "选用户:直接选本地用户", Result, h_acc_show_selector,
          Uid *out, const void *settings)
{
    (void)settings;
    if (out != NULL) {
        *out = k_uid;
    }
    return RES_OK;
}

HLE_FORCE("_ZN2nn7account22TryOpenPreselectedUserEPNS0_10UserHandleE",
          "预选用户 = 本地用户,恒成功", unsigned, h_acc_try_preselected, UserHandle *h)
{
    if (h != NULL) {
        h->v[0] = k_uid.v[0];
        h->v[1] = k_uid.v[1];
        h->v[2] = USER_HANDLE_MAGIC;
    }
    return 1;
}

HLE_FORCE("_ZN2nn7account8OpenUserEPNS0_10UserHandleERKNS0_3UidE",
          "Uid → UserHandle", Result, h_acc_open_user, UserHandle *h, const Uid *uid)
{
    if (h == NULL || uid == NULL) {
        return RES_FAIL;
    }
    h->v[0] = uid->v[0];
    h->v[1] = uid->v[1];
    h->v[2] = USER_HANDLE_MAGIC;
    return RES_OK;
}

HLE_FORCE("_ZN2nn7account9GetUserIdEPNS0_3UidERKNS0_10UserHandleE",
          "UserHandle → Uid(魔数校验)", Result, h_acc_get_uid, Uid *out, const UserHandle *h)
{
    if (out == NULL || !handle_valid(h)) {
        DMLOG_W("account: GetUserId 收到非法 UserHandle");
        return RES_FAIL;
    }
    out->v[0] = h->v[0];
    out->v[1] = h->v[1];
    return RES_OK;
}

HLE_FORCE("_ZN2nn7account11GetNicknameEPNS0_8NicknameERKNS0_3UidE",
          "昵称:DM_NICK 或 Player", Result, h_acc_nickname, char *nick, const Uid *uid)
{
    (void)uid;
    if (nick == NULL) {
        return RES_FAIL;
    }
    memset(nick, 0, 33);
    const char *n = getenv("DM_NICK");
    if (n == NULL || n[0] == 0) {
        n = "Player";
    }
    snprintf(nick, 33, "%s", n);
    return RES_OK;
}

/* ---------- 联网账号:一律失败(与上游一致) ---------- */
HLE_FORCE("_ZN2nn7account36EnsureNetworkServiceAccountAvailableERKNS0_10UserHandleE",
          "无联网账号", Result, h_acc_ensure_nsa, const UserHandle *h) { (void)h; return RES_FAIL; }

HLE_FORCE("_ZN2nn7account26GetNetworkServiceAccountIdEPNS0_23NetworkServiceAccountIdERKNS0_10UserHandleE",
          "无联网账号", Result, h_acc_nsa_id, void *out, const UserHandle *h)
{
    (void)out;
    (void)h;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account37LoadNetworkServiceAccountIdTokenCacheEPmPcmRKNS0_10UserHandleE",
          "无联网账号", Result, h_acc_nsa_token, uint64_t *n, char *buf, uint64_t sz,
          const UserHandle *h)
{
    (void)buf;
    (void)sz;
    (void)h;
    if (n != NULL) {
        *n = 0;
    }
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account40ShowLicenseRequirementsForNetworkServiceERKNS0_10UserHandleE",
          "无联网账号", Result, h_acc_license, const UserHandle *h) { (void)h; return RES_FAIL; }

HLE_FORCE("_ZN2nn7account12AsyncContextC1Ev", "不建会话", void, h_acc_async_ctor, void *self)
{
    (void)self;
}

HLE_FORCE("_ZN2nn7account44EnsureNetworkServiceAccountIdTokenCacheAsyncEPNS0_12AsyncContextERKNS0_10UserHandleE",
          "无联网账号", Result, h_acc_async_nsa, void *ctx, const UserHandle *h)
{
    (void)ctx;
    (void)h;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account12AsyncContext14GetSystemEventEPNS_2os11SystemEventE",
          "异步上下文事件:宿主不提供(异步接口均失败)", Result, h_acc_async_event,
          void *self, void *ev)
{
    (void)self;
    (void)ev;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account12AsyncContext9GetResultEv", "异步接口均失败",
          Result, h_acc_async_result, void *self) { (void)self; return RES_FAIL; }

HLE_FORCE("_ZN2nn7account42NintendoAccountAuthorizationRequestContextC1Ev",
          "不建会话", void, h_acc_auth_ctor, void *self) { (void)self; }

HLE_FORCE("_ZN2nn7account42NintendoAccountAuthorizationRequestContextD1Ev",
          "空", void, h_acc_auth_dtor, void *self) { (void)self; }

HLE_FORCE("_ZN2nn7account41CreateNintendoAccountAuthorizationRequestEPNS0_42NintendoAccountAuthorizationRequestContextERKNS0_10UserHandleERKNS0_45NintendoAccountAuthorizationRequestParametersEPvm",
          "无任天堂账号授权", Result, h_acc_auth_create, void *ctx, const UserHandle *h,
          const void *prm, void *buf, uint64_t sz)
{
    (void)ctx;
    (void)h;
    (void)prm;
    (void)buf;
    (void)sz;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account42NintendoAccountAuthorizationRequestContext14GetSystemEventEPNS_2os11SystemEventE",
          "无授权请求事件", Result, h_acc_auth_event, void *self, void *ev)
{
    (void)self;
    (void)ev;
    return RES_FAIL;
}

HLE_FORCE("_ZN2nn7account42NintendoAccountAuthorizationRequestContext34GetResultWithInteractionIfRequiredEv",
          "无授权请求", Result, h_acc_auth_result, void *self) { (void)self; return RES_FAIL; }

HLE_FORCE("_ZN2nn7account42NintendoAccountAuthorizationRequestContext10GetIdTokenEPmPcm",
          "无 id token", Result, h_acc_auth_token, void *self, uint64_t *n, char *buf, uint64_t sz)
{
    (void)self;
    (void)buf;
    (void)sz;
    if (n != NULL) {
        *n = 0;
    }
    return RES_FAIL;
}

int dmAccountSelfTest(char *report, int report_size)
{
    Uid u;
    UserHandle h;
    int n = -1;
    char nick[40] = {0};
    int ok = h_acc_list_users(&n, &u, 1) == RES_OK && n == 1 &&
             h_acc_try_preselected(&h) == 1 && h_acc_get_uid(&u, &h) == RES_OK &&
             h_acc_nickname(nick, &u) == RES_OK;
    snprintf(report, report_size,
             "account: 本地用户=%s nick=%s nsa=%s", ok ? "OK" : "FAIL", nick,
             h_acc_ensure_nsa(&h) == RES_FAIL ? "不可用(预期)" : "?");
    return ok ? 0 : -1;
}
