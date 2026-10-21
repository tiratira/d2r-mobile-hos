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
#include "gfx/nvn/dm_nvn.h"

#include <signal.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <ucontext.h>
#include <unistd.h>

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

    /* 客体对象现场:寄存器里的 callee-saved 常是游戏对象/表指针。
     * 用 process_vm_readv 读自身内存,读不到就跳过(不会二次 fault)。 */
    {
        static const int idxs[] = {19, 20, 21, 22, 23, 25, 28};
        for (unsigned k = 0; k < sizeof(idxs) / sizeof(idxs[0]); k++) {
            int r = idxs[k];
            unsigned long a = regs[r];
            if (a < 0x1000 || a > 0x7fffffffffULL) {
                continue;
            }
            const dm_module_t *mm = dm_module_of(a);
            unsigned long w[4] = {0}, e[2] = {0};
            struct iovec l1 = {w, sizeof(w)}, r1 = {(void *)a, sizeof(w)};
            ssize_t n1 = syscall(SYS_process_vm_readv, getpid(), &l1, 1, &r1, 1, 0);
            struct iovec l2 = {e, sizeof(e)}, r2 = {(void *)(a + 0xE8), sizeof(e)};
            ssize_t n2 = syscall(SYS_process_vm_readv, getpid(), &l2, 1, &r2, 1, 0);
            if (n1 == (ssize_t)sizeof(w)) {
                log_line("OBJ x%d=%lx %s+%lx [0]=%lx [8]=%lx [16]=%lx [24]=%lx +E8=%lx/%lx",
                         r, a, mm ? mm->name : "-",
                         mm ? a - (unsigned long)(uintptr_t)mm->base : 0UL,
                         w[0], w[1], w[2], w[3],
                         n2 == (ssize_t)sizeof(e) ? e[0] : 0UL,
                         n2 == (ssize_t)sizeof(e) ? e[1] : 0UL);
            }
        }
    }

    /* 引擎在 guest 数据段缓存 NVN 函数指针(运行时由 nvnBootstrapLoader /
     * nvnDeviceGetProcAddress 写入)。崩溃前它用两个槽的返回值当分配器参数,
     * 这里把槽值读出来并反查 NVN 名字。 */
    {
        const dm_module_t *mm = dm_module_find("main");
        if (mm != NULL) {
            const unsigned long guest_base = (unsigned long)(uintptr_t)mm->base;
            const unsigned long main_vaddr = 0x4c000UL;   /* __dm_guest_main_start */
            static const unsigned long slots[] = {0x325ADA0UL, 0x325AE08UL, 0x325AE10UL, 0x327ADA0UL, 0x327AE08UL, 0x327AE10UL};
            for (unsigned i = 0; i < sizeof(slots) / sizeof(slots[0]); i++) {
                unsigned long addr = guest_base + (slots[i] - main_vaddr);
                unsigned long v = 0;
                struct iovec l = {&v, sizeof(v)};
                struct iovec r = {(void *)addr, sizeof(v)};
                if (syscall(SYS_process_vm_readv, getpid(), &l, 1, &r, 1, 0) == (ssize_t)sizeof(v)) {
                    const char *nm = dmNvnNameForAddr((uint64_t)v);
                    log_line("NVNGOT slot=%lx val=%lx name=%s", slots[i], v, nm ? nm : "?");
                }
                unsigned long nb[4] = {0};
                struct iovec ln = {nb, sizeof(nb)};
                struct iovec rn = {(void *)(addr - 8), sizeof(nb)};
                if (syscall(SYS_process_vm_readv, getpid(), &ln, 1, &rn, 1, 0) == (ssize_t)sizeof(nb)) {
                    log_line("NVNGOT slot=%lx ctx=[-8]=%lx [+0]=%lx [+8]=%lx [+16]=%lx",
                             slots[i], nb[0], nb[1], nb[2], nb[3]);
                }
            }
        }
    }

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

