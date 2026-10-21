/*
 * crash_hos.c — 宿主/客体崩溃现场记录(HOS 调试用)。
 *
 * 真机上客体线程 SIGSEGV/SIGBUS/SIGABRT 时,Hiview 的 faultlog 只给 PC/LR
 * 与不可靠的回溯。这里装一个全进程信号处理器,先把「启动阶段 + 全部通用
 * 寄存器 + 帧指针链」用 FATAL 级别打到 hilog(FATAL 不受默认级别过滤),
 * 再把信号交回系统,保留原生崩场记录。
 *
 * 注意:信号处理器里只做日志与只读内存访问;不做分配/加锁。
 */
#define _GNU_SOURCE 1
#include "host/dm.h"

#include <signal.h>
#include <stdarg.h>
#include <ucontext.h>

#define CRASH_FP_MAX 12

static void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void log_line(const char *fmt, ...)
{
    /* hilog 的格式化参数较少;这里先拼成一行,再用 FATAL 打出去 */
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    OH_LOG_FATAL(LOG_APP, "%{public}s", buf);
}

static void crash_handler(int sig, siginfo_t *si, void *ctx)
{
    unsigned long fault = (unsigned long)(si ? si->si_addr : 0);
    unsigned long pc = 0, sp = 0, lr = 0, x29 = 0;
    unsigned long regs[31] = {0};
    unsigned long far = 0;

#ifdef __aarch64__
    ucontext_t *uc = (ucontext_t *)ctx;
    if (uc != NULL) {
        far = (unsigned long)uc->uc_mcontext.fault_address;
        pc = (unsigned long)uc->uc_mcontext.pc;
        sp = (unsigned long)uc->uc_mcontext.sp;
        for (int i = 0; i < 31; i++) {
            regs[i] = (unsigned long)uc->uc_mcontext.regs[i];
        }
        lr = regs[30];
        x29 = regs[29];
    }
#else
    (void)ctx;
    (void)far;
#endif

    log_line("CRASH sig=%d stage=%s addr=%lx far=%lx pc=%lx lr=%lx sp=%lx fp=%lx",
             sig, dmHostBootStage(), fault, far, pc, lr, sp, x29);
    log_line("CRASH x0=%lx x1=%lx x2=%lx x3=%lx x4=%lx x5=%lx x6=%lx x7=%lx",
             regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
    log_line("CRASH x8=%lx x9=%lx x10=%lx x11=%lx x12=%lx x13=%lx x14=%lx x15=%lx",
             regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15]);
    log_line("CRASH x16=%lx x17=%lx x18=%lx x19=%lx x20=%lx x21=%lx x22=%lx x23=%lx",
             regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23]);
    log_line("CRASH x24=%lx x25=%lx x26=%lx x27=%lx x28=%lx x29=%lx x30=%lx",
             regs[24], regs[25], regs[26], regs[27], regs[28], regs[29], regs[30]);

    /* 帧指针链:客体代码都带 x29 帧指针(stp x29,x30)。逐帧打 [fp]=上一帧、
     * [fp+8]=返回地址;指针明显越界就停,避免信号里再触发信号。 */
    unsigned long fp = x29;
    for (int i = 0; i < CRASH_FP_MAX && fp > 0x1000 && (fp & 7) == 0; i++) {
        unsigned long prev = 0, ret = 0;
        /* 只读;若未映射会再次 SIGSEGV,由默认行为结束(调试阶段可接受) */
        prev = *(volatile unsigned long *)fp;
        ret = *(volatile unsigned long *)(fp + 8);
        log_line("CRASH fp[%d]=%lx ret=%lx", i, fp, ret);
        if (prev <= fp) {
            break;   /* 栈向下增长;不严格递增即停 */
        }
        fp = prev;
    }

    /* 交给系统默认处理,保留 faultlog */
    signal(sig, SIG_DFL);
    raise(sig);
}

void dmCrashInstall(void)
{
    static int done;
    if (done) {
        return;
    }
    done = 1;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    DMLOG_I("crash handler installed");
}
