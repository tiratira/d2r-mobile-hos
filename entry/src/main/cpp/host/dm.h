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

/* hilog 的 OH_LOG_* 宏用 LOG_DOMAIN / LOG_TAG;不定义时 tag 为 NULL,
 * 设备上 `hilog | grep D2RMobileHOS` 就看不到本模块的日志。 */
#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0x0
#endif
#ifndef LOG_TAG
#define LOG_TAG "D2RMobileHOS"
#endif
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

/* ---------- HLE 登记段(与上游 hle.h 同构的 ELF 版) ----------
 * 两个链接段由 hle_core.c 的 __start_/__stop_ 边界符号遍历:
 *   swhle   : 导入解析表(HLE)—— 只在模块里「未定义」的符号会查到
 *             (服务客户端若在 sdk 里有定义,走下面的 swforce)。
 *   swforce : 强制接管表(HLE_FORCE)—— 装载重定位时优先于模块内定义,
 *             用于把 SDK 里的 Horizon 服务客户端整块换成本仓实现。
 * 每个强制接管条目必须给一句理由(装载后 hle_force_report 打到 hilog)。 */
typedef struct { const char *name; void *fn; } hle_ent;
typedef struct { const char *name; void *fn; const char *why; } hle_force_ent;

#define HLE(mangled, ret, cname, ...)                                            \
    static ret cname(__VA_ARGS__);                                               \
    __attribute__((used, section("swhle"), aligned(8)))                          \
    static const hle_ent hle_ent_##cname = {mangled, (void *)cname};             \
    static ret cname(__VA_ARGS__)

#define HLE_FORCE(mangled, why, ret, cname, ...)                                 \
    static ret cname(__VA_ARGS__);                                               \
    __attribute__((used, section("swforce"), aligned(8)))                        \
    static const hle_force_ent hle_force_##cname = {mangled, (void *)cname, why};\
    static ret cname(__VA_ARGS__)

uint64_t hle_lookup(const char *name);           /* swhle 表:0 = 没有 */
const void *dm_swforce_lookup(const char *name); /* swforce 表:NULL = 没有 */
uint64_t resolve_import(const char *name, int weak);  /* 未定义导入 → 接管/HLE/记名桩 */
void hle_force_report(void);
int hle_registered_counts(uint32_t *hle, uint32_t *force);

/* 每线程环境(hle_core.c) */
void dm_guest_thread_init(void);
uint64_t dm_guest_tls_get(void);
void dm_guest_tls_set(uint64_t v);
uint64_t dm_guest_tlr_get(void);
uint64_t dm_param_get(unsigned key_idx);
void dm_param_set(unsigned key_idx, uint64_t v);   /* oe 通知取走时把分辨率参数位推给客体线程 */

/* 门分派(hle_core.c;entry_hos.S 的 dm_gate 调入) */
uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs);
void gate_register_abs(uint64_t ret_abs, int kind, uint32_t arg, uint64_t orig_abs);
uint64_t unimpl_c(unsigned idx, uint64_t *regs);
void dm_thread_returned(void);
void dm_thread_enter(uint64_t entry, uint64_t arg, uint64_t sp);

/* svc 分派(hle_svc.c;线程/同步类在 hle_thread.c,宿主调用在 hle_hostcall.c) */
uint64_t svc_dispatch(uint32_t num, uint64_t *regs, uint64_t orig_addr);
int svc_thread(int num, uint64_t *regs);
uint64_t svc_host_call(uint32_t num, uint64_t *regs, uint64_t orig);
uint32_t dm_svc_thread_main_init(void);   /* 主线程登记,进客体前调一次 */
uint32_t dm_thr_cur_handle(void);
int dm_svc_selftest(char *report, int report_size);
int dm_guest_heap_readable(uint64_t a, size_t n);

/* 模块注册表(hle_core.c 登记;loader_hos.c 装载后补全动态段字段) */
typedef struct {
    char name[16];
    uint8_t *base;          /* 镜像首址(.so 节内,原地重定位) */
    uint64_t size;          /* image_size(未补齐) */
    /* 装载器字段(loader_hos.c) */
    const void *dyn;        /* Elf64_Dyn(镜像内) */
    const void *symtab;     /* Elf64_Sym */
    const char *strtab;
    uint32_t nsym;
    uint32_t *symh;         /* 名字→符号下标 开地址哈希 */
    uint32_t symh_mask;
    uint64_t init, init_array, init_arraysz;
    uint64_t patch_off, text_end, ro_off, data_off;
    int reloc_rel, reloc_abs, reloc_jmp;   /* 重定位计数(自检报告) */
} dm_module_t;
void dm_module_register(const char *name, const uint8_t *base, uint64_t size);
dm_module_t *dm_module_find(const char *name);
const dm_module_t *dm_module_of(uint64_t addr);
const dm_module_t *dm_module_iter(const dm_module_t *prev);   /* NULL = 从头遍历 */
int dm_max_modules(void);
dm_module_t *dm_module_at(int idx);

/* 装载器(host/loader_hos.c,扮演 Horizon rtld) */
int dm_load_module(const void *image, const void *meta, const char *name);
void dm_relocate_all(void);
uint8_t *dm_guest_sym(const char *name);
int dm_loader_report(char *buf, int size);
const void *dm_swforce_lookup(const char *name);
void run_module_inits(void);                  /* 各模块 DT_INIT(rtld InitModules 角色) */

/* 导入解析覆盖报告(loader_hos.c):装载/重定位后按符号名分类统计
 * (模块内定义 / swforce / swhle / 记名桩)。真机上「记名桩清单」就是
 * 下一批 HLE 的排期表;x86_64 data-only 构建同样可跑。 */
int dm_import_report(char *buf, int size);
int dm_import_stub_count(void);
int dm_import_total_count(void);

/* 门表统计/启动序(host_boot.c、host_start.c) */
uint32_t dm_gate_count(void);
int dmHostBoot(const char *which, char *report, int report_size);
int dmHostProbeModules(char *report, int report_size);
int dmHostStart(char *report, int report_size);   /* 真实 SDK 启动序;ARM64 才执行 */
const char *dmHostBootStage(void);                /* 最后到达的启动阶段(UI/hilog) */
void dmCrashInstall(void);                        /* SIGSEGV/ABRT 现场记录到 hilog */
void dmNvnInit(void);                             /* 登记宿主已实现的 NVN 入口 */

/* 服务层 HLE(host/hle_fs.c、hle_oe.c、hle_hid.c、hle_time.c) */
void dmFsInit(void);                                     /* 建挂载表(沙箱目录) */
void dmFsSetRoots(const char *romfs, const char *save);   /* 沙箱目录;NULL=保持 */
void dmServiceInitAll(void);                              /* fs/oe/hid/time 初始化钩子 */
int dmFsSelfTest(char *report, int report_size);
int dmOeSelfTest(char *report, int report_size);
int dmHidSelfTest(char *report, int report_size);
int dmTimeSelfTest(char *report, int report_size);
int dmMiscSelfTest(char *report, int report_size);
int dmAccountSelfTest(char *report, int report_size);
void dmOeSetOpMode(int docked);                             /* 0 掌机 / 1 主机 */
int dmOeOpMode(void);
void dmOeSetFocus(int state);                               /* 1 前台 / 2 失焦 / 3 后台 */
void dmHidSet(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry);

/* 音频(gfx 之外的 OHAudio 后端) */
int dmAudioSelfTest(char *report, int report_size);
int dmAudioPlayTone(int ms);
void dmAudioSuspend(int on);
const char *dmAudioReport(void);

/* 前端接口(front_hos) */
uint64_t front_game_res(int mode);   /* {u32 宽, u32 高} 打包;mode 0=掌机 1=主机 */

#ifdef __cplusplus
}
#endif

#endif /* DM_HOST_DM_H */
