/*
 * hle_thread.c — Horizon 内核的线程与同步 SVC(上游 dm/host/hle_thread.c 移植)。
 *
 * 每个客体线程 = 一个宿主 pthread,跑在 SDK 给的客体栈上;SDK 的 nn::os
 * 原样运行,只把它下面的「内核」换成宿主。语义按公开 Horizon 内核行为
 * (Atmosphère mesosphère 的 KConditionVariable / KAddressArbiter /
 * KSynchronization):所有等待挂在一把全局锁 G 上,每线程一个条件变量
 * (时钟取 CLOCK_MONOTONIC,超时语义与 Horizon 一致)。
 *
 * 上游移植改动:
 *   - 去 mach 线程端口 / QoS / 启动时间线 / 现场导出(诊断特性,后续按需平移)
 *   - 线程名读取从 vm_read_overwrite 改为同进程直接读(guest 与宿主同地址空间)
 *   - 条件变量用 pthread_condattr_setclock(CLOCK_MONOTONIC)
 *   - 条件变量的「先置键、再放锁」顺序照抄上游修复(并行负载下写者永久
 *     等待的坑,注释见 0x1C 分支)
 */
#include "host/dm.h"

#include <errno.h>
#include <sched.h>
#include <time.h>

#define R_TIMEOUT     0xEA01u   /* 内核模块 1,描述 117 */
#define R_CANCELLED   0xEC01u   /* 118 */
#define R_BAD_HANDLE  0xE401u   /* 114 */
#define R_BAD_STATE   0xFA01u   /* 125 */
#define WAITERS_BIT   0x40000000u
#define H_CUR_THREAD  0xFFFF8000u

enum { O_FREE, O_THREAD, O_EVENT_R, O_EVENT_W };
typedef struct event { int signaled; int refs; } event_t;
typedef struct thr {
    uint32_t handle; uint64_t id;
    uint64_t entry, arg, stack_top; int prio, core; uint64_t mask;
    int started, exited, cancel;
    pthread_t pt; pthread_cond_t cv;
    /* 当前等待:kind 0=无 1=锁 2=条件 3=地址 4=同步对象 */
    int wkind, woke; uint64_t waddr, wkey; uint32_t wtag; int64_t wres;
    uint64_t wseq;             /* 登记先后序号:同优先级先到先醒 */
    struct thr *next;          /* 等待链 */
    struct thr *all;           /* 全部线程链 */
    int last_svc; uint64_t last_svc_lr;
    int closed;                /* 句柄已关:与 exited 同时成立才释放对象 */
} thr_t;
static thr_t *g_all;

static pthread_mutex_t G = PTHREAD_MUTEX_INITIALIZER;
#define MAX_H 4096
static struct { int type; void *obj; } g_h[MAX_H];
static pthread_key_t g_keythr;
static uint64_t g_next_id = 0x51;
static thr_t *g_wait;          /* 全部在等的线程(锁/条件/地址/同步) */

static uint32_t h_new(int type, void *obj)
{
    for (int i = 1; i < MAX_H; i++) {
        if (!g_h[i].type) {
            g_h[i].type = type;
            g_h[i].obj = obj;
            return 0x1000 + i;
        }
    }
    die("句柄表满");
    return 0;
}

static void *h_peek(uint32_t h, int type)
{
    if (h == H_CUR_THREAD && type == O_THREAD) {
        return pthread_getspecific(g_keythr);
    }
    unsigned i = h - 0x1000;
    if (i == 0 || i >= MAX_H || g_h[i].type != type) {
        return NULL;
    }
    return g_h[i].obj;
}

static void *h_get(uint32_t h, int type)
{
    void *o = h_peek(h, type);
    if (!o) {
        LOG("句柄无效 %{public}#x(要类型 %{public}d)", h, type);
    }
    return o;
}

static thr_t *cur(void)
{
    thr_t *t = pthread_getspecific(g_keythr);
    if (!t) {
        die("非客体线程调用了线程 SVC");
    }
    return t;
}

/* ---------- 等待原语(持 G 调用) ---------- */
static void wait_unlink(thr_t *t)
{
    for (thr_t **p = &g_wait; *p; p = &(*p)->next) {
        if (*p == t) {
            *p = t->next;
            break;
        }
    }
    t->next = NULL;
}

/* 按内核顺序挑等待者:优先级数值小者优先,同优先级先登记者优先。
 * kind 1 按锁地址、2 按条件键、3 按地址匹配。 */
static uint64_t g_wseq;
static thr_t *pick(int kind, uint64_t a)
{
    thr_t *b = NULL;
    for (thr_t *t = g_wait; t; t = t->next) {
        if (t->wkind != kind || (kind == 2 ? t->wkey : t->waddr) != a) {
            continue;
        }
        if (!b || t->prio < b->prio || (t->prio == b->prio && t->wseq < b->wseq)) {
            b = t;
        }
    }
    return b;
}

static void wake(thr_t *t, int64_t res)
{
    wait_unlink(t);
    t->wkind = 0;
    t->woke = 1;
    t->wres = res;
    pthread_cond_signal(&t->cv);
}

static void cv_init_monotonic(pthread_cond_t *cv)
{
    pthread_condattr_t ca;
    pthread_condattr_init(&ca);
    pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
    pthread_cond_init(cv, &ca);
    pthread_condattr_destroy(&ca);
}

/* 挂起直到被唤醒或超时(ns<0 无限、0 立即超时);返回 wres 或 R_TIMEOUT */
static int64_t block(thr_t *t, int kind, int64_t ns)
{
    t->wkind = kind;
    t->woke = 0;
    t->next = g_wait;
    g_wait = t;
    t->wseq = ++g_wseq;
    if (ns == 0) {
        wait_unlink(t);
        t->wkind = 0;
        return R_TIMEOUT;
    }
    struct timespec dl;
    if (ns > 0) {
        clock_gettime(CLOCK_MONOTONIC, &dl);
        dl.tv_sec += ns / 1000000000;
        dl.tv_nsec += ns % 1000000000;
        if (dl.tv_nsec >= 1000000000) {
            dl.tv_sec++;
            dl.tv_nsec -= 1000000000;
        }
    }
    while (!t->woke) {
        int rc = ns > 0 ? pthread_cond_timedwait(&t->cv, &G, &dl) : pthread_cond_wait(&t->cv, &G);
        if (rc == ETIMEDOUT && !t->woke) {
            wait_unlink(t);
            t->wkind = 0;
            return R_TIMEOUT;
        }
    }
    return t->wres;
}

/* ---------- 锁(客体地址上的 32 位锁字) ---------- */
static inline uint32_t ld32(uint64_t a)
{
    return __atomic_load_n((uint32_t *)a, __ATOMIC_SEQ_CST);
}
static inline void st32(uint64_t a, uint32_t v)
{
    __atomic_store_n((uint32_t *)a, v, __ATOMIC_SEQ_CST);
}
static inline int cas32(uint64_t a, uint32_t *exp, uint32_t v)
{
    return __atomic_compare_exchange_n((uint32_t *)a, exp, v, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

/* 释放 addr 处的锁:交给等在它上面的第一个线程,或清零(持 G) */
static void lock_handoff(uint64_t addr)
{
    thr_t *w = pick(1, addr);
    int more = 0;
    if (!w) {
        st32(addr, 0);
        return;
    }
    for (thr_t *t = g_wait; t; t = t->next) {
        if (t != w && t->wkind == 1 && t->waddr == addr) {
            more = 1;
            break;
        }
    }
    st32(addr, w->wtag | (more ? WAITERS_BIT : 0));
    wake(w, 0);
}

/* 让 t 去拿 addr 处的锁:空闲就直接拿到并唤醒,否则置等待者位并转为锁等待(持 G) */
static void lock_acquire_for(thr_t *t, uint64_t addr, uint32_t tag)
{
    uint32_t v = ld32(addr);
    for (;;) {
        if (v == 0) {
            if (cas32(addr, &v, tag)) {
                wake(t, 0);
                return;
            }
            continue;
        }
        if (cas32(addr, &v, v | WAITERS_BIT)) {
            break;
        }
    }
    t->wkind = 1;
    t->waddr = addr;
    t->wtag = tag;             /* 仍在 g_wait 里,改为锁等待 */
    t->wseq = ++g_wseq;        /* 同优先级排在已有锁等待者之后 */
}

/* ---------- 线程 ---------- */
static void *thread_main(void *p)
{
    thr_t *t = p;
    pthread_setspecific(g_keythr, t);
    dm_guest_thread_init();
    pthread_mutex_lock(&G);
    while (!t->started) {
        pthread_cond_wait(&t->cv, &G);
    }
    pthread_mutex_unlock(&G);
    dm_thread_enter(t->entry, t->arg, t->stack_top);   /* 不返回:客体须经 ExitThread 退出 */
    die("客体线程入口返回");
    return NULL;
}

static void thr_free(thr_t *t)   /* 持 G */
{
    for (thr_t **p = &g_all; *p; p = &(*p)->all) {
        if (*p == t) {
            *p = t->all;
            break;
        }
    }
    pthread_cond_destroy(&t->cv);
    free(t);
}

/* 线程收尾用宿主栈:musl 的 tcache/TSD 析构需要足够的栈空间,
 * 客体线程栈可能只有十几 KiB,直接 pthread_exit 会越界。 */
#ifdef __aarch64__
void dm_thread_exit_to(void *host_sp) __attribute__((noreturn));
void dm_thread_exit_final(void);
static _Thread_local uint8_t g_thread_exit_stack[65536] __attribute__((aligned(16)));

void dm_thread_exit_final(void)
{
    pthread_exit(NULL);
}
#endif

static void thread_exit(thr_t *t)
{
    /* 不打日志:退出发生在客体线程自己的栈上(有的只有十几 KiB) */
    pthread_mutex_lock(&G);
    t->exited = 1;
    for (thr_t *w = g_wait; w;) {
        thr_t *n = w->next;
        if (w->wkind == 4) {
            wake(w, -1);       /* 同步等待者重查 */
        }
        w = n;
    }
    pthread_setspecific(g_keythr, NULL);
    if (t->closed) {
        thr_free(t);
    }
    pthread_mutex_unlock(&G);
#ifdef __aarch64__
    dm_thread_exit_to((void *)(g_thread_exit_stack + sizeof(g_thread_exit_stack)));
#else
    pthread_exit(NULL);
#endif
}

/* 主线程登记,返回其句柄(host_boot 在进客体前调) */
uint32_t dm_svc_thread_main_init(void)
{
    static int done;
    if (done) {
        return 0;
    }
    done = 1;
    pthread_key_create(&g_keythr, NULL);
    thr_t *t = calloc(1, sizeof *t);
    cv_init_monotonic(&t->cv);
    t->started = 1;
    t->prio = 44;
    t->core = 0;
    t->mask = 7;
    t->pt = pthread_self();
    pthread_mutex_lock(&G);
    t->handle = h_new(O_THREAD, t);
    t->id = g_next_id++;
    t->all = g_all;
    g_all = t;
    pthread_mutex_unlock(&G);
    pthread_setspecific(g_keythr, t);
    return t->handle;
}

/* 当前线程的客体句柄(0 = 未登记;信号安全只读 TSD) */
uint32_t dm_thr_cur_handle(void)
{
    thr_t *t = pthread_getspecific(g_keythr);
    return t ? t->handle : 0;
}

/* 线程名:SDK ThreadType+0x1a0 是名字指针(CreateThread 的 x2 = ThreadType*)。
 * 只在客体堆已开放范围内读(等价上游 vm_read_overwrite 的安全读),
 * 宿主地址(自检)或未开放区域回 "?"。 */
static const char *thr_name(thr_t *t, char *nb, size_t n)
{
    if (!t->arg || t->arg < 0x10000 || !dm_guest_heap_readable(t->arg + 0x1a0, 8)) {
        return t->handle == 0x1001 ? "主线程" : "?";
    }
    uint64_t np = 0;
    memcpy(&np, (void *)(uintptr_t)(t->arg + 0x1a0), 8);
    if (np < 0x10000 || !dm_guest_heap_readable(np, 1)) {
        return "?";
    }
    size_t i = 0;
    uint8_t *s = (uint8_t *)(uintptr_t)np;
    for (; i + 1 < n; i++) {
        uint8_t c = s[i];
        if (c == 0) {
            break;
        }
        nb[i] = (char)c;
    }
    nb[i] = 0;
    return nb;
}

/* ---------- 同步对象 ---------- */
static int sig_obj(uint32_t h, int *bad)   /* 1=已触发(持 G) */
{
    unsigned i = h - 0x1000;
    if (h == H_CUR_THREAD) {
        return 0;
    }
    if (i == 0 || i >= MAX_H) {
        *bad = 1;
        return 0;
    }
    switch (g_h[i].type) {
    case O_THREAD:
        return ((thr_t *)g_h[i].obj)->exited;
    case O_EVENT_R:
        return ((event_t *)g_h[i].obj)->signaled;
    default:
        *bad = 1;
        return 0;
    }
}

static void wake_sync_waiters(void)
{
    for (thr_t *w = g_wait; w;) {
        thr_t *n = w->next;
        if (w->wkind == 4) {
            wake(w, -1);
        }
        w = n;
    }
}

static int64_t now_mono_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ll + ts.tv_nsec;
}

/* 处理线程/同步类 SVC;返回 1 = 已处理 */
static int svc_thread_body(int num, uint64_t *regs);

#ifdef __aarch64__
uint64_t dm_host_stack_run(uint64_t sp_top, uint64_t fn, uint64_t a, uint64_t b);
static _Thread_local uint8_t g_svc_host_stack[65536] __attribute__((aligned(16)));
#endif

/* svc 处理函数跑在客体线程的小栈上(十几 KiB),内部再 pthread_create/malloc
 * 会栈溢出;整段切到 64 KiB 宿主栈执行。 */
int svc_thread(int num, uint64_t *regs)
{
#ifdef __aarch64__
    return (int)dm_host_stack_run((uint64_t)(uintptr_t)(g_svc_host_stack + sizeof(g_svc_host_stack)),
                                  (uint64_t)(uintptr_t)&svc_thread_body,
                                  (uint64_t)(uintptr_t)num, (uint64_t)(uintptr_t)regs);
#else
    return svc_thread_body(num, regs);
#endif
}

static int svc_thread_body(int num, uint64_t *regs)
{
    {
        thr_t *me = pthread_getspecific(g_keythr);
        if (me) {
            me->last_svc = num;
            me->last_svc_lr = regs[30];
        }
    }
    switch (num) {
    case 0x08: {   /* CreateThread(→w1 句柄; x1 入口, x2 参数, x3 栈顶, w4 优先级, w5 核) */
        thr_t *t = calloc(1, sizeof *t);
        cv_init_monotonic(&t->cv);
        t->entry = regs[1];
        t->arg = regs[2];
        t->stack_top = regs[3] & ~0xFull;
        t->prio = (int)regs[4];
        t->core = (int)regs[5];
        t->mask = t->core >= 0 ? 1ull << t->core : 7;
        pthread_mutex_lock(&G);
        t->handle = h_new(O_THREAD, t);
        t->id = g_next_id++;
        t->all = g_all;
        g_all = t;
        pthread_mutex_unlock(&G);
        pthread_attr_t a;
        pthread_attr_init(&a);
        pthread_attr_setstacksize(&a, 1 << 20);
        pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);   /* 客体经句柄等待,不 join */
        if (pthread_create(&t->pt, &a, thread_main, t)) {
            die("pthread_create 失败");
        }
        pthread_attr_destroy(&a);
        char nb[32];
        LOG("CreateThread h=%{public}#x「%{public}s」entry=%{public}llx sp=%{public}llx prio=%{public}d",
            t->handle, thr_name(t, nb, sizeof nb), (unsigned long long)t->entry,
            (unsigned long long)t->stack_top, t->prio);
        regs[0] = 0;
        regs[1] = t->handle;
        return 1;
    }
    case 0x09: {   /* StartThread(w0) */
        pthread_mutex_lock(&G);
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) {
            t->started = 1;
            pthread_cond_signal(&t->cv);
        }
        pthread_mutex_unlock(&G);
        regs[0] = t ? 0 : R_BAD_HANDLE;
        return 1;
    }
    case 0x0A:
        thread_exit(cur());
        return 1;
    case 0x0B: {   /* SleepThread(x0 ns);0/-1/-2 = 各类让出 */
        int64_t ns = (int64_t)regs[0];
        if (ns > 0) {
            struct timespec ts = {ns / 1000000000, ns % 1000000000};
            nanosleep(&ts, 0);
        } else {
            sched_yield();
        }
        return 1;
    }
    case 0x0C: {   /* GetThreadPriority(→w1; w1 句柄) */
        thr_t *t = h_get((uint32_t)regs[1], O_THREAD);
        regs[0] = t ? 0 : R_BAD_HANDLE;
        regs[1] = t ? (uint64_t)t->prio : 0;
        return 1;
    }
    case 0x0D: {   /* SetThreadPriority(w0 句柄, w1 优先级) */
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) {
            t->prio = (int)regs[1];
        }
        regs[0] = t ? 0 : R_BAD_HANDLE;
        return 1;
    }
    case 0x0E: {   /* GetThreadCoreMask(→w1 理想核, x2 掩码; w2 句柄) */
        thr_t *t = h_get((uint32_t)regs[2], O_THREAD);
        regs[0] = t ? 0 : R_BAD_HANDLE;
        regs[1] = t ? (uint64_t)(uint32_t)t->core : 0;
        regs[2] = t ? t->mask : 0;
        return 1;
    }
    case 0x0F: {   /* SetThreadCoreMask(w0, w1 理想核, x2 掩码):宿主只记账 */
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) {
            int c = (int)regs[1];
            if (c >= 0) {
                t->core = c;
            } else if (c == -1) {
                t->core = 0;
            }
            if (regs[2]) {
                t->mask = regs[2];
            }
        }
        regs[0] = t ? 0 : R_BAD_HANDLE;
        return 1;
    }
    case 0x10:    /* GetCurrentProcessorNumber */
        regs[0] = (uint64_t)(uint32_t)cur()->core;
        return 1;
    case 0x25: {  /* GetThreadId(→x1; w1 句柄) */
        thr_t *t = h_get((uint32_t)regs[1], O_THREAD);
        regs[0] = t ? 0 : R_BAD_HANDLE;
        regs[1] = t ? t->id : 0;
        return 1;
    }

    case 0x1A: {   /* ArbitrateLock(w0 持有者句柄, x1 锁地址, w2 本线程标签) */
        thr_t *t = cur();
        uint64_t addr = regs[1];
        pthread_mutex_lock(&G);
        int64_t r = 0;
        if (ld32(addr) == ((uint32_t)regs[0] | WAITERS_BIT)) {
            t->waddr = addr;
            t->wtag = (uint32_t)regs[2];
            r = block(t, 1, -1);
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        return 1;
    }
    case 0x1B:    /* ArbitrateUnlock(x0 锁地址) */
        pthread_mutex_lock(&G);
        lock_handoff(regs[0]);
        pthread_mutex_unlock(&G);
        regs[0] = 0;
        return 1;
    case 0x1C: {  /* WaitProcessWideKeyAtomic(x0 锁地址, x1 条件键, w2 标签, x3 超时) */
        thr_t *t = cur();
        uint64_t addr = regs[0], key = regs[1];
        pthread_mutex_lock(&G);
        /* 顺序要紧:先置条件键 = 1,再放互斥锁。SDK 的 Signal 在用户态先读
         * 条件键、为 0 就不发 0x1D;用户态抢互斥锁也不经过 G。若先放锁后置
         * 键,别的线程可在用户态抢到锁、改完条件、读到键 0 就不唤醒,本线
         * 程随后挂上去永远等不到(上游在并行负载下实测过的死锁)。挂进等
         * 待链在 block 里、仍持 G:0x1D 要取 G,不会落在这之前。 */
        st32(key, 1);
        t->wkey = key;
        t->waddr = addr;
        t->wtag = (uint32_t)regs[2];
        lock_handoff(addr);
        int64_t r = block(t, 2, (int64_t)regs[3]);   /* 超时不重新拿锁(用户态自己拿) */
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        return 1;
    }
    case 0x1D: {  /* SignalProcessWideKey(x0 条件键, w1 个数,<=0 全部) */
        uint64_t key = regs[0];
        int cnt = (int)regs[1], n = 0;
        pthread_mutex_lock(&G);
        for (thr_t *w; (cnt <= 0 || n < cnt) && (w = pick(2, key)); n++) {
            lock_acquire_for(w, w->waddr, w->wtag);   /* 按内核顺序;转为锁等待后不再是 kind 2 */
        }
        int left = 0;
        for (thr_t *w = g_wait; w; w = w->next) {
            if (w->wkind == 2 && w->wkey == key) {
                left = 1;
                break;
            }
        }
        if (!left) {
            st32(key, 0);
        }
        pthread_mutex_unlock(&G);
        regs[0] = 0;
        return 1;
    }

    case 0x34: {  /* WaitForAddress(x0 地址, w1 类型, w2 值, x3 超时):0 小于则等、1 小于则减一再等、2 相等则等 */
        thr_t *t = cur();
        uint64_t addr = regs[0];
        int type = (int)regs[1];
        int32_t val = (int32_t)regs[2];
        pthread_mutex_lock(&G);
        int64_t r;
        uint32_t v = ld32(addr);
        int ok = type == 2 ? (int32_t)v == val : (int32_t)v < val;
        if (ok && type == 1) {
            while (!cas32(addr, &v, v - 1)) {
                if (!((int32_t)v < val)) {
                    ok = 0;
                    break;
                }
            }
        }
        if (!ok) {
            r = R_BAD_STATE;
        } else {
            t->waddr = addr;
            r = block(t, 3, (int64_t)regs[3]);
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        return 1;
    }
    case 0x35: {  /* SignalToAddress(x0 地址, w1 类型, w2 值, w3 个数) */
        uint64_t addr = regs[0];
        int type = (int)regs[1];
        int32_t val = (int32_t)regs[2];
        int cnt = (int)regs[3];
        pthread_mutex_lock(&G);
        int nw = 0;
        for (thr_t *w = g_wait; w; w = w->next) {
            if (w->wkind == 3 && w->waddr == addr) {
                nw++;
            }
        }
        int64_t r = 0;
        if (type != 0) {
            int32_t nv = val + 1;
            if (type == 2) {
                if (cnt <= 0) {
                    nv = nw > 0 ? val - 1 : val + 1;
                } else {
                    nv = nw == 0 ? val + 1 : nw <= cnt ? val - 1 : val;
                }
            }
            uint32_t e = (uint32_t)val;
            if (!cas32(addr, &e, (uint32_t)nv)) {
                r = R_BAD_STATE;
            }
        }
        if (!r) {
            int n = 0;
            for (thr_t *w; (cnt <= 0 || n < cnt) && (w = pick(3, addr)); n++) {
                wake(w, 0);
            }
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        return 1;
    }

    case 0x45: {  /* CreateEvent(→w1 写端, w2 读端) */
        event_t *e = calloc(1, sizeof *e);
        e->refs = 2;
        pthread_mutex_lock(&G);
        uint32_t hw = h_new(O_EVENT_W, e);
        uint32_t hr = h_new(O_EVENT_R, e);
        pthread_mutex_unlock(&G);
        regs[0] = 0;
        regs[1] = hw;
        regs[2] = hr;
        return 1;
    }
    case 0x11:    /* SignalEvent(写端) */
    case 0x12:    /* ClearEvent(任一端) */
    case 0x17: {  /* ResetSignal(读端,未触发报错) */
        pthread_mutex_lock(&G);
        uint32_t h = (uint32_t)regs[0];
        event_t *e = num == 0x11 ? h_peek(h, O_EVENT_W)
                   : num == 0x17 ? h_peek(h, O_EVENT_R)
                   : (h_peek(h, O_EVENT_W) ? h_peek(h, O_EVENT_W) : h_peek(h, O_EVENT_R));
        int64_t r = 0;
        if (!e) {
            r = (num == 0x17 && h_peek(h, O_THREAD)) ? R_BAD_STATE : R_BAD_HANDLE;
            if (r == R_BAD_HANDLE) {
                LOG("事件 SVC %{public}#x 句柄无效 %{public}#x", num, h);
            }
        } else if (num == 0x11) {
            e->signaled = 1;
            wake_sync_waiters();
        } else if (num == 0x17 && !e->signaled) {
            r = R_BAD_STATE;
        } else {
            e->signaled = 0;
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        return 1;
    }
    case 0x18: {  /* WaitSynchronization(→w1 下标; x1 句柄数组, w2 个数, x3 超时) */
        thr_t *t = cur();
        const uint32_t *hs = (const uint32_t *)regs[1];
        int n = (int)regs[2];
        int64_t ns = (int64_t)regs[3];
        int64_t deadline = ns > 0 ? now_mono_ns() + ns : 0;
        pthread_mutex_lock(&G);
        int64_t r;
        int idx = -1;
        for (;;) {
            if (t->cancel) {
                t->cancel = 0;
                r = R_CANCELLED;
                break;
            }
            int bad = 0;
            for (int i = 0; i < n && idx < 0; i++) {
                if (sig_obj(hs[i], &bad)) {
                    idx = i;
                }
            }
            if (bad) {
                r = R_BAD_HANDLE;
                break;
            }
            if (idx >= 0) {
                r = 0;
                break;
            }
            int64_t left = ns < 0 ? -1 : ns == 0 ? 0 : deadline - now_mono_ns();
            if (ns > 0 && left <= 0) {
                r = R_TIMEOUT;
                break;
            }
            if (block(t, 4, left) == R_TIMEOUT) {
                r = R_TIMEOUT;
                break;
            }
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r;
        regs[1] = idx >= 0 ? (uint64_t)idx : 0;
        return 1;
    }
    case 0x19: {  /* CancelSynchronization(w0 线程) */
        pthread_mutex_lock(&G);
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) {
            t->cancel = 1;
            if (t->wkind == 4) {
                wake(t, -1);
            }
        }
        pthread_mutex_unlock(&G);
        regs[0] = t ? 0 : R_BAD_HANDLE;
        return 1;
    }
    case 0x16: {  /* CloseHandle(w0):事件两端都关才释放;线程对象随最后引用消失 */
        pthread_mutex_lock(&G);
        unsigned i = (uint32_t)regs[0] - 0x1000;
        if (i > 0 && i < MAX_H && (g_h[i].type == O_EVENT_R || g_h[i].type == O_EVENT_W)) {
            event_t *e = g_h[i].obj;
            if (--e->refs == 0) {
                free(e);
            }
            g_h[i].type = O_FREE;
            g_h[i].obj = NULL;
        } else if (i > 0 && i < MAX_H && g_h[i].type == O_THREAD) {
            thr_t *t = g_h[i].obj;
            g_h[i].type = O_FREE;
            g_h[i].obj = NULL;
            t->closed = 1;
            if (t->exited) {
                thr_free(t);
            }
        }
        pthread_mutex_unlock(&G);
        regs[0] = 0;
        return 1;
    }
    }
    return 0;
}


