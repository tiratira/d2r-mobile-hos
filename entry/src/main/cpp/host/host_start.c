/*
 * host_start.c — 真实 nnMain 启动链(上游 dm/host/main.c 的 guest_main 的 HOS 版)。
 *
 * 装载器(loader_hos.c)已经把三个真实模块原地重定位、符号解析、门表注册完;
 * 本文件按 nn::init::Start(sdk+0x2b0470)的真实顺序初始化 SDK 运行时,再进
 * nnMain:
 *
 *   _init_libc0 → nnosInitialize(hmain,0) → _init_libc1
 *   → nninitInitializeSdkModule → nninitStartup → _init_libc2
 *   → 各模块 DT_INIT(init_array)→ SetHostArgc/Argv → nnMain
 *
 * x86_64(PC 模拟器)构建只做预检:三模块装载、启动序关键符号、init_array、
 * 门表/服务接管覆盖、未解析导入清单 —— 同一套装载/解析代码,只是不执行
 * ARM64 客体代码(真机才执行)。
 */
#include "host/dm.h"

#include <pthread.h>
#include <stdarg.h>

/* 启动阶段(UI 轮询 / hilog 对拍用) */
static char s_stage[96] = "未启动";

const char *dmHostBootStage(void)
{
    return s_stage;
}

static void stage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_stage, sizeof(s_stage), fmt, ap);
    va_end(ap);
    DMLOG_I("BOOT %{public}s", s_stage);
}

/* 上游 guest_main 逐条照抄的符号名(argv[0] 固定 D2R) */
static const char *const k_boot_syms[] = {
    "_init_libc0",
    "nnosInitialize",
    "_init_libc1",
    "nninitInitializeSdkModule",
    "nninitStartup",
    "_init_libc2",
    "_ZN2nn2os11SetHostArgcEi",
    "_ZN2nn2os11SetHostArgvEPPc",
    "nnMain",
};
#define BOOT_SYM_COUNT ((int)(sizeof(k_boot_syms) / sizeof(k_boot_syms[0])))

static char *s_argv[] = {(char *)"D2R", NULL};
static int s_argc = 1;

#ifdef __aarch64__
static void *guest_boot_thread(void *arg)
{
    (void)arg;
    dm_guest_thread_init();
    dmServiceInitAll();

    void *sym[BOOT_SYM_COUNT];
    for (int i = 0; i < BOOT_SYM_COUNT; i++) {
        sym[i] = dm_guest_sym(k_boot_syms[i]);
        if (sym[i] == NULL) {
            stage("缺符号 %s,启动中止", k_boot_syms[i]);
            return NULL;
        }
    }
    void (*init0)(void) = (void (*)(void))sym[0];
    void (*nnosInit)(uint64_t, uint64_t) = (void (*)(uint64_t, uint64_t))sym[1];
    void (*init1)(void) = (void (*)(void))sym[2];
    void (*initSdk)(void) = (void (*)(void))sym[3];
    void (*startup)(void) = (void (*)(void))sym[4];
    void (*init2)(void) = (void (*)(void))sym[5];
    void (*setArgc)(void *) = (void (*)(void *))sym[6];
    void (*setArgv)(void *) = (void (*)(void *))sym[7];
    void (*nnMain)(void) = (void (*)(void))sym[8];

    uint32_t hmain = dm_svc_thread_main_init();   /* 主线程句柄:nnosInitialize 的 x0 */
    /* Horizon 新线程 x18 初值 = 0(客体自写自读的 TCB);宿主 pthread 的 x18 是
     * 平台寄存器,不一定是 0,进客体前先清零,否则 libc 启动期读到宿主 x18
     * 会当自己的 TCB 解引用(真机实测:进游戏即 SIGSEGV @PC=0)。 */
#if defined(__aarch64__)
    __asm__ volatile("mov x18, xzr" ::: "x18");
#endif
    stage("_init_libc0");
    init0();
    stage("nnosInitialize");
    nnosInit(hmain, 0);
    stage("_init_libc1");
    init1();
    stage("nninitInitializeSdkModule");
    initSdk();
    stage("nninitStartup");
    startup();
    stage("_init_libc2");
    init2();
    stage("各模块 DT_INIT");
    run_module_inits();
    stage("SetHostArgc/Argv");
    setArgc((void *)(uintptr_t)s_argc);
    setArgv(s_argv);
    stage("nnMain");
    nnMain();
    stage("nnMain returned");
    return NULL;
}
#endif

/* 预检:启动序需要的符号、init_array、门表、导入覆盖 */
static int boot_preflight(char *report, int report_size)
{
    char imp[512];
    dm_import_report(imp, sizeof(imp));
    uint32_t nhle = 0, nforce = 0;
    hle_registered_counts(&nhle, &nforce);

    int found = 0;
    char miss[128] = "";
    int moff = 0;
    for (int i = 0; i < BOOT_SYM_COUNT; i++) {
        if (dm_guest_sym(k_boot_syms[i]) != NULL) {
            found++;
        } else if (moff < (int)sizeof(miss)) {
            moff += snprintf(miss + moff, sizeof(miss) - moff, "%s,", k_boot_syms[i]);
        }
    }
    int nmod = dm_max_modules();
    int init_mods = 0;
    unsigned long long init_entries = 0;
    for (int i = 0; i < nmod; i++) {
        dm_module_t *m = dm_module_at(i);
        if (m->init) {
            init_mods++;
        }
        init_entries += m->init_arraysz / 8;
    }
    int stubs = dm_import_stub_count();
    snprintf(report, report_size,
             "启动预检:%d 模块 | 启动序符号 %d/%d | DT_INIT %d 模块/%llu 项 | 门 %u | "
             "接管 HLE %u/force %u | %s | 未解析导入桩 %d",
             nmod, found, BOOT_SYM_COUNT, init_mods, init_entries, dm_gate_count(),
             (unsigned)nhle, (unsigned)nforce, imp, stubs);
    if (found != BOOT_SYM_COUNT) {
        DMLOG_E("启动预检缺符号:%{public}s", miss);
        return -1;
    }
    return 0;
}

int dmHostStart(char *report, int report_size)
{
    /* 1) 确认三模块已装载/重定位(幂等) */
    char mods[320];
    dmCrashInstall();
    dmHostProbeModules(mods, sizeof(mods));
    dmServiceInitAll();
    hle_force_report();

    /* 2) 预检(两种 ABI 都跑) */
    int pf = boot_preflight(report, report_size);
    if (pf != 0) {
        char tmp[512];
        snprintf(tmp, sizeof(tmp), "%s", report);
        snprintf(report, report_size, "启动预检 FAILED:%s", tmp);
        snprintf(s_stage, sizeof(s_stage), "%s", "预检失败");
        return -1;
    }
    char pre[640];
    snprintf(pre, sizeof(pre), "%s", report);

#ifdef __aarch64__
    static int started;
    if (started) {
        snprintf(report, report_size, "启动已在进行:%s", s_stage);
        return 0;
    }
    started = 1;
    stage("创建客体主线程");
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8 << 20);
    pthread_t th;
    int rc = pthread_create(&th, &attr, guest_boot_thread, NULL);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        stage("客体主线程创建失败 %d", rc);
        snprintf(report, report_size, "启动线程创建失败(%d):%s", rc, s_stage);
        return -1;
    }
    pthread_detach(th);
    snprintf(report, report_size, "%s | ARM64:已起客体主线程,阶段:%s", pre, s_stage);
    return 0;
#else
    snprintf(report, report_size,
             "%s | x86_64:预检通过,真实执行仅限 ARM64 真机(dm_gate/NVN 走同一套码)", pre);
    snprintf(s_stage, sizeof(s_stage), "%s", "预检通过(x86_64 不执行)");
    return 0;
#endif
}
