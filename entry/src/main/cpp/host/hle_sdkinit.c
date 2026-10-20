/*
 * hle_sdkinit.c — 服务层初始化接口的强制接管(上游 dm/host/hle_sdkinit.c 的 HOS 版)。
 *
 * nninitInitializeSdkModule 会去连 Horizon 的各系统服务(IPC);宿主没有这些服务进程,
 * 所以这些 Initialize 一律接成空操作,后续真正用到的接口再按 hle_*.c 逐个接管
 * (文件接口见 hle_fs.c,时间见 hle_time.c,运行环境见 hle_oe.c,手柄见 hle_hid.c)。
 *
 * 注意:这些符号在 sdk 模块里有定义,必须走 HLE_FORCE(重定位时覆盖面定义),
 * 不能走 HLE(只对未定义导入生效)。
 */
#include "host/dm.h"

void dmFsInit(void);
void dmOeInit(void);
void dmHidInit(void);
void dmTimeInit(void);

static void once(const char *what)
{
    DMLOG_I("init 接管:%{public}s(空操作)", what);
}

HLE_FORCE("nndiagStartup", "连 lm 日志服务;诊断日志改由宿主直接输出",
          void, h_nndiagStartup, void) { once("nndiagStartup"); }

HLE_FORCE("_ZN2nn2oe10InitializeEv", "连 am/oe 运行环境服务;运行环境查询逐个接成宿主默认值",
          void, h_oe_Initialize, void) { dmOeInit(); once("oe::Initialize"); }

HLE_FORCE("_ZN2nn3aoc10InitializeEv", "追加内容(DLC)服务:不建会话",
          void, h_aoc_Initialize, void) { once("aoc::Initialize"); }

HLE_FORCE("_ZN2nn3err26InitializeContextWriterApiEv", "err 错误上下文服务;宿主不上报",
          void, h_err_InitCtx, void) { once("err::InitializeContextWriterApi"); }

HLE_FORCE("_ZN2nn2fs24InitializeForApplicationEv", "连 fsp-srv 文件服务;文件接口接宿主沙箱 + RomFS",
          void, h_fs_InitApp, void) { dmFsInit(); once("fs::InitializeForApplication"); }

HLE_FORCE("_ZN2nn4pctl10InitializeEv", "pctl 家长控制服务;单机无家长控制",
          void, h_pctl_Initialize, void) { once("pctl::Initialize"); }

HLE_FORCE("_ZN2nn4diag34InitializeApplicationAbortObserverEv",
          "注册到系统的中止观察;宿主崩溃由信号处理打印",
          void, h_diag_AbortObs, void) { once("diag::InitializeApplicationAbortObserver"); }

/* hle_*.c 的服务初始化钩子:即使对应 Initialize 的接管条目尚未落位,
 * 启动序也能先建好宿主侧的挂载表/手柄表。 */
void dmServiceInitAll(void)
{
    dmFsInit();
    dmOeInit();
    dmHidInit();
    dmTimeInit();
}
