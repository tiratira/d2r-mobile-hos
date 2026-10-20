/*
 * hle_time.c — nn::time 时间服务 HLE(上游 dm/host/hle_time.c 的 HOS 最小集)。
 *
 * Horizon 的 time 服务提供「标准用户系统时钟」;SDK 自带 libc 的时区数据不全
 * (localtime 恒 UTC),必须把 localtime/localtime_r/mktime 也接宿主,原版界面
 * 的本地时刻(编年史发现时间等)才跟系统时区一致。
 *
 * SDK 的 time() 是"开机时长",不能直接换墙钟 —— 上游只在一处返回地址
 * (建局种子)特判 DM_FIXED_TIME,其余原样转调 SDK。这里同样转调,找不到
 * 原实现才退回墙钟。
 */
#include "host/dm.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int16_t year;   /* 已 +1900 */
    int8_t month;   /* 已 +1 */
    int8_t day;
    int8_t hour;
    int8_t minute;
    int8_t second;
    int8_t pad;
} CalendarTime;

typedef struct {
    uint32_t wday;
    uint32_t yday;
    char tzname[8];
    uint32_t dst;
    int32_t offset;   /* 秒 */
} CalendarAdditionalInfo;

void dmTimeInit(void)
{
    static int done;
    if (done) {
        return;
    }
    done = 1;
    tzset();
    DMLOG_I("time: 初始化(系统时区)");
}

static int64_t fixed_time(void)
{
    static int64_t v = -1;
    if (v < 0) {
        const char *e = getenv("DM_FIXED_TIME");
        v = e != NULL ? (int64_t)strtoll(e, NULL, 10) : 0;
    }
    return v;
}

/* SDK 的 time():开机时长,不是墙钟 —— 转调它保持游戏计时语义 */
HLE_FORCE("time", "SDK 的 time 是开机时长;DM_FIXED_TIME 时才钉墙钟,其余转调原实现",
          int64_t, h_time, int64_t *out)
{
    int64_t t = fixed_time();
    if (t == 0) {
        static int64_t (*orig)(int64_t *);
        if (orig == NULL) {
            orig = (int64_t (*)(int64_t *))dm_guest_sym("time");
        }
        t = orig != NULL ? orig(out) : (int64_t)time(NULL);
    }
    if (out != NULL) {
        *out = t;
    }
    return t;
}

HLE_FORCE("_ZN2nn4time10InitializeEv", "time 服务初始化:宿主直接用系统时钟",
          Result, h_time_init, void) { dmTimeInit(); return RES_OK; }

HLE_FORCE("_ZN2nn4time13IsInitializedEv", "始终已初始化",
          int, h_time_isinit, void) { return 1; }

HLE_FORCE("_ZN2nn4time23StandardUserSystemClock14GetCurrentTimeEPNS0_9PosixTimeE",
          "标准用户系统时钟:墙钟秒(存档头 +0x20 的存档时刻)",
          Result, h_time_get_current, int64_t *out)
{
    if (out == NULL) {
        return RES_FAIL;
    }
    int64_t t = fixed_time();
    *out = t != 0 ? t : (int64_t)time(NULL);
    return RES_OK;
}

HLE_FORCE("_ZN2nn4time23StandardUserSystemClock3nowEv", "同上,按值返回",
          int64_t, h_time_now, void)
{
    int64_t t = fixed_time();
    return t != 0 ? t : (int64_t)time(NULL);
}

HLE_FORCE("_ZN2nn4time23StandardUserSystemClock9to_time_tERKNSt3__16chrono10time_pointINS0_17SystemClockTraitsENS3_8durationIlNS2_5ratioILl1ELl1EEEEEEE",
          "time_point → 秒:同值", int64_t, h_time_to_time_t, const int64_t *tp)
{
    return tp != NULL ? *tp : 0;
}

HLE_FORCE("_ZN2nn4time14ToCalendarTimeEPNS0_12CalendarTimeEPNS0_22CalendarAdditionalInfoERKNS0_9PosixTimeE",
          "绝对秒 → 本地日历时刻(宿主 localtime_r)",
          Result, h_time_to_calendar, CalendarTime *ct, CalendarAdditionalInfo *ai,
          const int64_t *pt)
{
    if (ct == NULL || ai == NULL || pt == NULL) {
        return RES_FAIL;
    }
    time_t t = (time_t)*pt;
    struct tm tmv;
    memset(ct, 0, sizeof(*ct));
    memset(ai, 0, sizeof(*ai));
    if (localtime_r(&t, &tmv) == NULL) {
        return RES_FAIL;
    }
    ct->year = (int16_t)(tmv.tm_year + 1900);
    ct->month = (int8_t)(tmv.tm_mon + 1);
    ct->day = (int8_t)tmv.tm_mday;
    ct->hour = (int8_t)tmv.tm_hour;
    ct->minute = (int8_t)tmv.tm_min;
    ct->second = (int8_t)tmv.tm_sec;
    ai->wday = (uint32_t)tmv.tm_wday;
    ai->yday = (uint32_t)tmv.tm_yday;
    ai->dst = (uint32_t)(tmv.tm_isdst > 0 ? 1 : 0);
    ai->offset = (int32_t)tmv.tm_gmtoff;
    const char *zn = tmv.tm_zone != NULL ? tmv.tm_zone : (tzname[0] ? tzname[0] : "UTC");
    snprintf(ai->tzname, sizeof(ai->tzname), "%s", zn);
    return RES_OK;
}

/* SDK 自带 localtime 无时区数据 → 接宿主。形参是"指向 time_t 的指针"。 */
HLE_FORCE("localtime", "SDK 自带 localtime 恒 UTC;接宿主系统时区",
          struct tm *, h_localtime, const int64_t *t)
{
    if (t == NULL) {
        return NULL;
    }
    time_t tt = (time_t)*t;
    return localtime(&tt);
}

HLE_FORCE("localtime_r", "同上,可重入版", struct tm *, h_localtime_r,
          const int64_t *t, struct tm *out)
{
    if (t == NULL || out == NULL) {
        return NULL;
    }
    time_t tt = (time_t)*t;
    return localtime_r(&tt, out);
}

HLE_FORCE("mktime", "本地日历 → 绝对秒(宿主 mktime)",
          int64_t, h_mktime, struct tm *tm)
{
    return tm != NULL ? (int64_t)mktime(tm) : 0;
}

int dmTimeSelfTest(char *report, int report_size)
{
    dmTimeInit();
    int64_t now = (int64_t)time(NULL);
    CalendarTime ct;
    CalendarAdditionalInfo ai;
    int ok = h_time_to_calendar(&ct, &ai, &now) == RES_OK &&
             ct.year >= 2020 && ct.month >= 1 && ct.month <= 12;
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %s%s",
             ct.year, ct.month, ct.day, ct.hour, ct.minute, ct.second,
             ai.tzname, ai.dst ? "(DST)" : "");
    snprintf(report, report_size, "time: now=%s | ToCalendarTime=%s", buf,
             ok ? "OK" : "FAIL");
    return ok ? 0 : -1;
}
