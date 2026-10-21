/*
 * hle_os_event.c — nn::os::SystemEvent 的 HLE。
 *
 * 真机实测:friends/aoc 等 HLE 返回的“假 SystemEvent”被引擎交给 SDK 的
 * nn::os::TryWaitSystemEvent,SDK 校验对象/句柄后 diag::Abort。
 * 这里把 SystemEvent 的等待/信号/创建入口全部剪成宿主语义:
 * 创建=成功但对象为空;等待/超时等待=未触发;信号/清除=空操作。
 */
#include "host/dm.h"

typedef uint32_t Result;

HLE_FORCE("_ZN2nn2os18TryWaitSystemEventEPNS0_15SystemEventTypeE", "SystemEvent 由宿主接管:未触发",
          int, h_ev_trywait, void *ev) { (void)ev; return 0; }

HLE_FORCE("_ZN2nn2os17WaitSystemEventEPNS0_15SystemEventTypeE", "SystemEvent 由宿主接管:立即返回",
          void, h_ev_wait, void *ev) { (void)ev; }

HLE_FORCE("_ZN2nn2os20TimedWaitSystemEventEPNS0_15SystemEventTypeENS_8TimeSpanE", "SystemEvent 由宿主接管:超时",
          int, h_ev_timedwait, void *ev, uint64_t span) { (void)ev; (void)span; return 0; }

HLE_FORCE("_ZN2nn2os17SignalSystemEventEPNS0_15SystemEventTypeE", "SystemEvent 由宿主接管:空操作",
          void, h_ev_signal, void *ev) { (void)ev; }

HLE_FORCE("_ZN2nn2os16ClearSystemEventEPNS0_15SystemEventTypeE", "SystemEvent 由宿主接管:空操作",
          void, h_ev_clear, void *ev) { (void)ev; }

HLE_FORCE("_ZN2nn2os17CreateSystemEventEPNS0_15SystemEventTypeENS0_14EventClearModeEb", "SystemEvent 由宿主接管:成功",
          Result, h_ev_create, void *ev, int mode, int initial)
{
    (void)mode; (void)initial;
    if (ev != NULL) {
        *(uint64_t *)ev = 0;
    }
    return 0;
}

HLE_FORCE("_ZN2nn2os18DestroySystemEventEPNS0_15SystemEventTypeE", "SystemEvent 由宿主接管:空操作",
          void, h_ev_destroy, void *ev) { (void)ev; }
