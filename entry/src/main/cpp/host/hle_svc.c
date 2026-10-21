/*
 * hle_svc.c — Horizon 内核系统调用:内存布局 + 堆 + 查询 + 调试输出 + 计时
 * (上游 dm/host/hle_svc.c 移植),线程/同步类委派给 hle_thread.c,
 * 0x80 起的宿主调用(剪口约定)委派给 hle_hostcall.c。
 *
 * 寄存器约定:入参 w1.. / 出参 w0=Result、x1=值(GetInfo / SetHeapSize),
 * 与公开 Horizon ABI 一致。
 *
 * 上游移植改动:
 *   - 客体地址空间预留:mmap PROT_NONE MAP_NORESERVE,Linux/OHOS 虚拟地址
 *     充裕,保留上游 5120→4864 MiB 降档重试逻辑作保险
 *   - 主线程栈回报:pthread_getattr_np(Apple 用 pthread_get_stackaddr_np)
 *   - 计时:dmPlatCounterRead(hal 层 19.2 MHz 语义)
 */
#define _GNU_SOURCE
#include "host/dm.h"
#include "hal/dm_platform.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/* ---------- 客体地址空间(按 Horizon 分区,地址 = 宿主真地址) ---------- */
static size_t g_reserve;           /* 逐档降级后实际预留的大小 */
static uint8_t *g_as;              /* 整块保留的基址 */
static uint8_t *g_heap;
static size_t g_heap_cur, g_heap_cap;
static int g_heap_rejects;
static uint8_t *g_alias;
static size_t g_alias_cap;
static uint8_t *g_stack;
static size_t g_stack_cap;

static size_t dm_pagesize(void)
{
    static size_t ps;
    if (!ps) {
        long v = sysconf(_SC_PAGESIZE);
        ps = v > 0 ? (size_t)v : 0x1000;
    }
    return ps;
}

/* 地址段整个落在客体堆当前已开放范围内:免陷核对的直接读判定 */
int dm_guest_heap_readable(uint64_t a, size_t n)
{
    uint64_t b = (uint64_t)(uintptr_t)g_heap;
    size_t cur = g_heap_cur;
    return g_heap && a >= b && n <= cur && a - b <= cur - n;
}

static void mem_init(void)
{
    if (g_as) {
        return;
    }
    const size_t start = 5ull << 30, step = 128ull << 20, limit = 4864ull << 20;
    for (size_t sz = start; sz >= limit; sz -= step) {
        g_as = mmap(NULL, sz, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (g_as != MAP_FAILED) {
            g_reserve = sz;
            break;
        }
        if (sz == limit) {
            die("reserve guest AS: mmap 全档失败");
        }
    }
    /* 固定划分(都在预留块内):alias 512MB、stack 256MB、heap 其余(基址 +1GB 起) */
    g_alias = g_as + 0x10000000;
    g_alias_cap = 0x20000000;
    g_stack = g_as + 0x30000000;
    g_stack_cap = 0x10000000;
    g_heap = g_as + 0x40000000;
    g_heap_cap = g_reserve - 0x40000000;
    LOG("guest AS %{public}p +%{public}zu MiB heap=%{public}p alias=%{public}p stack=%{public}p",
        g_as, g_reserve >> 20, g_heap, g_alias, g_stack);
}

/* 应用内存池 = Switch(4 GB 机型)给应用的 3285 MB + 512 MB(DM_APP_POOL_ADD_MB 可调)。
 * 游戏启动把 QueryMemoryInfo 报的总量整块设堆、失败减 2 MB 重试,交给自己的
 * StandardAllocator —— 堆大小 = 池 − 映像,宿主报多少它就只有多少。 */
#define APP_POOL 0xCD500000ull
static uint64_t app_pool(void)
{
    static uint64_t v;
    if (!v) {
        const char *e = getenv("DM_APP_POOL_ADD_MB");
        long add = e ? atol(e) : 512;
        v = APP_POOL + ((uint64_t)add << 20);
    }
    return v;
}

static size_t image_bytes(void)
{
    size_t n = 0;
    for (const dm_module_t *m = dm_module_iter(NULL); m; m = dm_module_iter(m)) {
        n += (size_t)m->size;
    }
    return (n + 0x1fffff) & ~(size_t)0x1fffff;
}

/* 已映射区间登记(MapMemory 栈等),QueryMemory 据此回报;SDK 靠它找空位 */
#define MAX_MAP 4096
static struct { uint64_t base, size, src; uint32_t state; } g_map[MAX_MAP];
static int g_nmap;
static pthread_mutex_t g_map_lock = PTHREAD_MUTEX_INITIALIZER;
static int map_find(uint64_t a, uint64_t n)   /* 返回 1 基下标,0 = 无重叠 */
{
    for (int i = 0; i < g_nmap; i++) {
        if (a < g_map[i].base + g_map[i].size && g_map[i].base < a + n) {
            return i + 1;
        }
    }
    return 0;
}
static void map_add(uint64_t b, uint64_t n, uint64_t src, uint32_t st)
{
    pthread_mutex_lock(&g_map_lock);
    if (g_nmap >= MAX_MAP) {
        die("映射表满");
    }
    g_map[g_nmap++] = (typeof(g_map[0])){b, n, src, st};
    pthread_mutex_unlock(&g_map_lock);
}
static void map_del(int i)
{
    pthread_mutex_lock(&g_map_lock);
    g_map[i] = g_map[--g_nmap];
    pthread_mutex_unlock(&g_map_lock);
}

enum { I_CoreMask = 0, I_PriorityMask = 1, I_AliasAddr = 2, I_AliasSize = 3,
       I_HeapAddr = 4, I_HeapSize = 5, I_TotalMem = 6, I_UsedMem = 7,
       I_DebuggerAttached = 8, I_ResourceLimit = 9, I_IdleTick = 10,
       I_RandomEntropy = 11, I_AslrAddr = 12, I_AslrSize = 13,
       I_StackAddr = 14, I_StackSize = 15, I_SysResTotal = 16, I_SysResUsed = 17,
       I_ProgramId = 18, I_InitProcIdRange = 19, I_UserExcAddr = 20,
       I_TotalNonSys = 21, I_UsedNonSys = 22, I_IsApplication = 23 };

static uint8_t g_seen[256];

static uint64_t entropy64(void)
{
    uint64_t v = 0;
    int fd = open("/dev/urandom", 0);
    if (fd >= 0) {
        ssize_t r = read(fd, &v, sizeof v);
        (void)r;
        close(fd);
    }
    if (!v) {   /* 兜底:时钟混合 */
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        v = (uint64_t)ts.tv_nsec * 0x9E3779B97F4A7C15ull ^ (uint64_t)ts.tv_sec;
    }
    return v;
}

extern uint64_t svc_host_call(uint32_t num, uint64_t *regs, uint64_t orig);

uint64_t svc_dispatch(uint32_t num, uint64_t *regs, uint64_t orig_addr)
{
    mem_init();
    if (svc_thread(num, regs)) {
        return regs[0];
    }
    if (num >= 0x80 && svc_host_call(num, regs, orig_addr)) {
        return regs[0];
    }
    switch (num) {
    case 0x01: {   /* SetHeapSize(→x1 堆基址; x1 大小) */
        size_t size = regs[1];
        if (size > g_heap_cap) {
            LOG("SetHeapSize OOM: req %{public}#zx > cap %{public}#zx", size, g_heap_cap);
            regs[0] = 0xD401;
            return 0;
        }
        if (size + image_bytes() > app_pool()) {
            g_heap_rejects++;   /* 游戏启动按 2 MB 递减重试,属正常路径 */
            regs[0] = 0x10801;
            return 0;
        }
        int rc = 0;
        if (size > g_heap_cur) {
            rc = mprotect(g_heap + g_heap_cur, size - g_heap_cur, PROT_READ | PROT_WRITE);
        } else if (size < g_heap_cur) {
            rc = mprotect(g_heap + size, g_heap_cur - size, PROT_NONE);
        }
        if (rc) {
            LOG("SetHeapSize mprotect errno=%{public}d", errno);
        }
        {
            static int said;
            if (!said && size) {
                said = 1;
                LOG("SetHeapSize 首次设堆 %{public}zu MiB(池 %{public}llu MiB − 映像 %{public}zu MiB;此前被拒 %{public}d 次)",
                    size >> 20, (unsigned long long)(app_pool() >> 20), image_bytes() >> 20, g_heap_rejects);
            }
        }
        g_heap_cur = size;
        regs[0] = 0;
        regs[1] = (uint64_t)(uintptr_t)g_heap;
        return 0;
    }
    case 0x02: {   /* SetMemoryPermission(addr, size, perm):游戏运行时改代码页权限打热补丁(实测写 main+0xa14ddc 函数序言) */
        uint64_t addr = regs[0], size = regs[1];
        uint32_t perm = (uint32_t)regs[2];
        if (size == 0) {
            regs[0] = 0;
            return 0;
        }
        int prot = 0;
        if (perm & 1) prot |= PROT_READ;
        if (perm & 2) prot |= PROT_WRITE;
        if (perm & 4) prot |= PROT_EXEC;
        size_t ps2 = dm_pagesize();
        uint64_t lo = addr & ~(uint64_t)(ps2 - 1);
        uint64_t hi = (addr + size + ps2 - 1) & ~(uint64_t)(ps2 - 1);
        if (mprotect((void *)(uintptr_t)lo, (size_t)(hi - lo), prot) != 0) {
            /* 空桩时代游戏已带病运行;失败仍回成功只留日志,避免新回归 */
            LOG("SetMemoryPermission mprotect(%{public}llx+%{public}llx→%{public}x) errno=%{public}d",
                (unsigned long long)addr, (unsigned long long)size, perm, errno);
            regs[0] = 0;
            return 0;
        }
        if (prot & PROT_EXEC) {
            /* 补丁发生在可写阶段,切回可执行后必须清 icache,否则宿主核可能执行旧指令 */
            __builtin___clear_cache((char *)(uintptr_t)lo, (char *)(uintptr_t)hi);
        }
        regs[0] = 0;
        return 0;
    }
    case 0x03:   /* SetMemoryAttribute */
        regs[0] = 0;
        return 0;
    case 0x04: {   /* MapMemory(dst, src, size):SDK 建线程时把堆上的栈搬进栈区 */
        uint64_t dst = regs[0], src = regs[1], size = regs[2];
        if ((uint8_t *)dst < g_stack || (uint8_t *)dst + size > g_stack + g_stack_cap || map_find(dst, size)) {
            LOG("MapMemory 目标非法或已占用 %{public}llx+%{public}llx",
                (unsigned long long)dst, (unsigned long long)size);
            regs[0] = 0xDC01;
            return 0;
        }
        size_t ps = dm_pagesize();
        uint64_t lo = dst & ~(uint64_t)(ps - 1);
        uint64_t hi = (dst + size + ps - 1) & ~(uint64_t)(ps - 1);
        if (mprotect((void *)lo, hi - lo, PROT_READ | PROT_WRITE)) {
            die("MapMemory mprotect errno 失败");
        }
        memcpy((void *)dst, (void *)src, size);   /* 搬移语义:映射期间源区不被访问 */
        map_add(dst, size, src, 0x0B /*MappedMemory*/);
        regs[0] = 0;
        return 0;
    }
    case 0x05: {   /* UnmapMemory(dst, src, size):内容搬回源区 */
        uint64_t dst = regs[0], src = regs[1], size = regs[2];
        int k = map_find(dst, size);
        pthread_mutex_lock(&g_map_lock);
        int ok = k && g_map[k - 1].base == dst && g_map[k - 1].size == size && g_map[k - 1].src == src;
        pthread_mutex_unlock(&g_map_lock);
        if (!ok) {
            LOG("UnmapMemory 与映射不符 %{public}llx+%{public}llx",
                (unsigned long long)dst, (unsigned long long)size);
            regs[0] = 0xD401;
            return 0;
        }
        memcpy((void *)src, (void *)dst, size);
        map_del(k - 1);
        regs[0] = 0;
        return 0;
    }
    case 0x06: {   /* QueryMemory(MemoryInfo* out, PageInfo* pout, addr) → x1 = pageinfo */
        uint64_t mi = regs[0], addr = regs[2];
        uint8_t *a = (uint8_t *)addr;
        uint64_t base, size, state, perm;
        int k;
        pthread_mutex_lock(&g_map_lock);
        if (a >= g_heap && a < g_heap + g_heap_cur) {
            base = (uint64_t)(uintptr_t)g_heap; size = g_heap_cur; state = 5 /*Heap*/; perm = 3;
        } else if ((k = map_find(addr, 1))) {
            base = g_map[k - 1].base; size = g_map[k - 1].size; state = g_map[k - 1].state; perm = 3;
        } else if (a >= g_as && a < g_as + g_reserve) {
            /* 空闲:到相邻映射/堆边界为止 */
            uint64_t lo = (uint64_t)(uintptr_t)g_as, hi = (uint64_t)(uintptr_t)g_as + g_reserve;
            if (a >= g_heap) {
                lo = (uint64_t)(uintptr_t)g_heap + g_heap_cur;
            } else if (hi > (uint64_t)(uintptr_t)g_heap) {
                hi = (uint64_t)(uintptr_t)g_heap;
            }
            for (int i = 0; i < g_nmap; i++) {
                uint64_t e = g_map[i].base + g_map[i].size;
                if (e <= addr && e > lo) {
                    lo = e;
                }
                if (g_map[i].base > addr && g_map[i].base < hi) {
                    hi = g_map[i].base;
                }
            }
            base = lo; size = hi - lo; state = 0; perm = 0;
        } else {
            /* 地址落在当前宿主线程栈里:回报整段栈(状态 0xB = 栈)。SDK 初始
             * 化主线程记录时拿当时的 sp 查这里,把回报区间当主线程栈。 */
            pthread_attr_t at;
            void *stop = NULL;
            size_t ssz = 0;
            pthread_getattr_np(pthread_self(), &at);
            pthread_attr_getstack(&at, &stop, &ssz);
            pthread_attr_destroy(&at);
            if (addr < (uint64_t)stop && addr >= (uint64_t)stop - ssz) {
                base = (uint64_t)stop - ssz; size = ssz; state = 0xB; perm = 3;
            } else {
                base = addr & ~0xFFFull; size = 0x1000; state = 3 /*Code*/; perm = 5;
            }
        }
        pthread_mutex_unlock(&g_map_lock);
        if (mi) {
            uint64_t *m = (uint64_t *)mi;
            m[0] = base; m[1] = size;
            ((uint32_t *)mi)[4] = (uint32_t)state;
            ((uint32_t *)mi)[5] = 0;
            ((uint32_t *)mi)[6] = (uint32_t)perm;
            ((uint32_t *)mi)[7] = 0;
            ((uint32_t *)mi)[8] = 0;
        }
        regs[0] = 0;
        regs[1] = 0;
        return 0;
    }
    case 0x2C:   /* MapPhysicalMemory(addr, size):只许在 alias 区内(SDK VammManager) */
    case 0x2D: { /* UnmapPhysicalMemory(addr, size) */
        uint8_t *a = (uint8_t *)(uintptr_t)regs[0];
        size_t size = (size_t)regs[1];
        if (a < g_alias || a + size > g_alias + g_alias_cap) {
            LOG("SVC %{public}#x 越出 alias 区 %{public}p+%{public}zx", num, a, size);
            regs[0] = 0xD401;
            return 0;
        }
        int rc = num == 0x2C ? mprotect(a, size, PROT_READ | PROT_WRITE)
                             : mprotect(a, size, PROT_NONE);
        if (rc) {
            die("SVC 0x2C/0x2D mprotect 失败");
        }
        regs[0] = 0;
        return 0;
    }
    case 0x26: {   /* Break(reason, ptr, size):SDK 致命中止 */
        uint32_t res = (regs[1] && regs[2] >= 4) ? *(uint32_t *)(uintptr_t)regs[1] : 0;
        const dm_module_t *mm = dm_module_of(orig_addr);
        LOG("Break reason=%{public}llx Result=%{public}#x svc 位于 %{public}s+%{public}llx",
            (unsigned long long)regs[0], res, mm ? mm->name : "?",
            mm ? (unsigned long long)(orig_addr - (uint64_t)(uintptr_t)mm->base) : 0ull);
        /* 客体调用链(x29 帧指针):定位是哪个 SDK 函数走到的 Break */
        uint64_t fp = regs[29];
        for (int i = 0; i < 16 && fp > 0x1000 && (fp & 7) == 0; i++) {
            uint64_t prev = *(volatile uint64_t *)(uintptr_t)fp;
            uint64_t ret = *(volatile uint64_t *)(uintptr_t)(fp + 8);
            const dm_module_t *rm = dm_module_of(ret);
            LOG("  bt[%{public}d] ret=%{public}llx %{public}s+%{public}llx", i,
                (unsigned long long)ret, rm ? rm->name : "?",
                rm ? (unsigned long long)(ret - (uint64_t)(uintptr_t)rm->base) : 0ull);
            if (prev <= fp) {
                break;
            }
            fp = prev;
        }
        if (regs[0] & 0x80000000ull) {
            regs[0] = 0;
            return 0;   /* 仅通知,可继续 */
        }
        _exit(42);
        return 0;
    }
    case 0x1E:   /* GetSystemTick → x0(19.2 MHz) */
        regs[0] = dmPlatCounterRead();
        return 0;
    case 0x27: {   /* OutputDebugString(str, len) */
        char buf[256];
        size_t n = regs[2] < sizeof(buf) - 1 ? (size_t)regs[2] : sizeof(buf) - 1;
        if (regs[1] && n) {
            memcpy(buf, (void *)(uintptr_t)regs[1], n);
            buf[n] = 0;
            LOG("[guest] %{public}s", buf);
        }
        regs[0] = 0;
        return 0;
    }
    case 0x29: {   /* GetInfo(out*, id, handle, subid) → x1 = 值 */
        uint64_t id = regs[1], v = 0;
        switch (id) {
        case I_CoreMask: v = 7; break;
        case I_PriorityMask: v = 0; break;
        case I_AliasAddr: v = (uint64_t)(uintptr_t)g_alias; break;
        case I_AliasSize: v = g_alias_cap; break;
        case I_HeapAddr:  v = (uint64_t)(uintptr_t)g_heap;  break;
        case I_HeapSize:  v = g_heap_cap;  break;
        case I_AslrAddr:  v = (uint64_t)(uintptr_t)g_as;    break;
        case I_AslrSize:  v = g_reserve;   break;
        case I_StackAddr: v = (uint64_t)(uintptr_t)g_stack; break;
        case I_StackSize: v = g_stack_cap; break;
        case I_TotalMem: case I_TotalNonSys: v = app_pool(); break;
        case I_UsedMem:  case I_UsedNonSys:  v = g_heap_cur + image_bytes(); break;
        case I_SysResTotal: v = 0x2000000; break;
        case I_SysResUsed:  v = 0x400000; break;
        case I_DebuggerAttached: v = 0; break;
        case I_RandomEntropy: v = entropy64(); break;
        case I_ProgramId: v = 0x0100726014352000ull; break;
        case I_IsApplication: v = 1; break;
        case I_UserExcAddr: v = 0; break;
        default: v = 0; break;
        }
        regs[0] = 0;
        regs[1] = v;
        return 0;
    }
    default:
        if (!g_seen[num & 0xff]) {
            g_seen[num & 0xff] = 1;
            const dm_module_t *mm = dm_module_of(orig_addr);
            LOG("SVC %{public}#x x0=%{public}llx x1=%{public}llx x2=%{public}llx(占位返回成功) 调用点 %{public}s+%{public}llx",
                num, (unsigned long long)regs[0], (unsigned long long)regs[1],
                (unsigned long long)regs[2], mm ? mm->name : "?",
                mm ? (unsigned long long)(orig_addr - (uint64_t)(uintptr_t)mm->base) : 0ull);
            /* 0x1f / 0x21 很可能是 IPC(ConnectToNamedPort/SendSyncRequest):
             * 把 x1 当字符串试着打出来,便于定位未接管的服务名。 */
            if (num == 0x1f || num == 0x21) {
                const char *s = (const char *)(uintptr_t)regs[1];
                if (s) {
                    char nm[64];
                    int ok = 1;
                    for (int i = 0; i < (int)sizeof(nm) - 1; i++) {
                        char ch = s[i];
                        if (ch == 0) { nm[i] = 0; break; }
                        if ((unsigned char)ch < 0x20 || (unsigned char)ch > 0x7e) { ok = 0; break; }
                        nm[i] = ch;
                        nm[i + 1] = 0;
                    }
                    if (ok) {
                        LOG("  SVC %{public}#x 字符串: \"%{public}s\"", num, nm);
                    }
                }
            }
        }
        regs[0] = 0;
        return 0;
    }
}

/* ---------- svc HLE 自检矩阵(宿主函数当客体入口,x86_64 模拟器同样可跑) ----------
 * 主线程全部走阻塞等待(2s 超时),子线程每阶段前睡 20ms:主线程在上一步
 * 唤醒后微秒级即达下一个阻塞点,20ms 裕量下时序确定。 */
#define R_TIMEOUT_T 0xEA01u

typedef struct {
    volatile uint32_t lockw;      /* ArbitrateLock 锁字 */
    volatile uint32_t keyw;       /* 条件键 */
    volatile uint32_t addrw;      /* 地址仲裁字 */
} st_sh_t;

static uint64_t st_svc(uint32_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                       uint64_t *x1out)
{
    uint64_t regs[31];
    memset(regs, 0, sizeof regs);
    regs[0] = a0; regs[1] = a1; regs[2] = a2; regs[3] = a3;
    svc_dispatch(num, regs, 0);
    if (x1out) {
        *x1out = regs[1];
    }
    return regs[0];
}

static void selftest_thread(uint64_t argp)
{
    st_sh_t *sh = (st_sh_t *)(uintptr_t)argp;
    struct timespec ts = {0, 20 * 1000000};
    nanosleep(&ts, 0);                                    /* 主线程已挂在 0x1A */
    st_svc(0x1B, (uint64_t)&sh->lockw, 0, 0, 0, NULL);    /* 锁交接给主线程 */
    nanosleep(&ts, 0);                                    /* 主线程已挂在 0x1C */
    st_svc(0x1D, (uint64_t)&sh->keyw, 1, 0, 0, NULL);     /* 唤主线程条件等待 */
    nanosleep(&ts, 0);                                    /* 主线程已挂在 0x34 */
    st_svc(0x35, (uint64_t)&sh->addrw, 0, 7, 1, NULL);    /* 唤主线程地址等待 */
    nanosleep(&ts, 0);                                    /* 主线程已挂 WaitSync */
    st_svc(0x0A, 0, 0, 0, 0, NULL);                       /* ExitThread */
}

int dm_svc_selftest(char *report, int report_size)
{
    st_sh_t sh;
    memset(&sh, 0, sizeof sh);
    int pass = 0, total = 0;
    uint64_t x1 = 0;

#define ST_CHECK(desc, cond)                                    \
    do {                                                        \
        total++;                                                \
        if (cond) {                                             \
            pass++;                                             \
        } else {                                                \
            LOG("svc 自检失败:%{public}s", desc);               \
        }                                                       \
    } while (0)

    dm_svc_thread_main_init();
    mem_init();

    /* 内存类:SetHeapSize / QueryMemory / GetInfo */
    uint64_t heapbase = 0;
    uint64_t r = st_svc(0x01, 0, 64ull << 20, 0, 0, &heapbase);
    ST_CHECK("SetHeapSize 64MiB", r == 0 && heapbase != 0);
    uint64_t mi[6];
    memset(mi, 0xAA, sizeof mi);
    r = st_svc(0x06, (uint64_t)(uintptr_t)mi, 0, heapbase + 0x1000, 0, NULL);
    ST_CHECK("QueryMemory(heap) state=5", r == 0 && mi[0] == heapbase && ((uint32_t *)mi)[4] == 5);
    uint64_t got = 0;
    r = st_svc(0x29, 0, I_HeapAddr, 0, 0, &got);
    ST_CHECK("GetInfo(HeapAddr) 一致", r == 0 && got == heapbase);
    r = st_svc(0x29, 0, I_ProgramId, 0, 0, &got);
    ST_CHECK("GetInfo(ProgramId)", r == 0 && got == 0x0100726014352000ull);
    uint64_t tick = st_svc(0x1E, 0, 0, 0, 0, NULL);   /* 0x1E:x0 直接是 tick,非 Result */
    ST_CHECK("GetSystemTick 走", tick > 0 && tick != (uint64_t)-1);

    /* 事件:建、触发、无限等待立即醒、清后超时(全确定序,不涉子线程) */
    uint64_t ev_w = 0, ev_r = 0;
    uint64_t regs2[31];
    memset(regs2, 0, sizeof regs2);
    svc_dispatch(0x45, regs2, 0);
    ev_w = regs2[1];
    ev_r = regs2[2];
    ST_CHECK("CreateEvent 两端", ev_w != 0 && ev_r != 0);
    r = st_svc(0x11, ev_w, 0, 0, 0, NULL);
    ST_CHECK("SignalEvent", r == 0);
    uint32_t hs[1] = { (uint32_t)ev_r };
    r = st_svc(0x18, 0, (uint64_t)(uintptr_t)hs, 1, (uint64_t)-1, &x1);
    ST_CHECK("WaitSync 已触发即醒", r == 0 && x1 == 0);
    r = st_svc(0x12, ev_r, 0, 0, 0, NULL);
    ST_CHECK("ClearEvent", r == 0);
    r = st_svc(0x18, 0, (uint64_t)(uintptr_t)hs, 1, 80 * 1000000, &x1);
    ST_CHECK("WaitSync 未触发超时", r == R_TIMEOUT_T);
    r = st_svc(0x16, ev_w, 0, 0, 0, NULL);
    r = st_svc(0x16, ev_r, 0, 0, 0, NULL);
    ST_CHECK("CloseHandle(event)", r == 0);

    /* 线程 + 锁 + 条件 + 地址仲裁:子线程 = 宿主函数入口,给真实栈 */
    void *stk = mmap(NULL, 256 * 1024, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    sh.lockw = 0x1001 | 0x40000000u;   /* 主线程标签持锁 + 等待者位 */
    sh.addrw = 7;
    uint64_t th = 0;
    r = st_svc(0x08, 0, (uint64_t)(uintptr_t)selftest_thread, (uint64_t)(uintptr_t)&sh,
               stk ? (uint64_t)stk + 256 * 1024 - 256 : 0, &th);
    (void)r;
    ST_CHECK("CreateThread", th != 0);
    uint64_t regs3[31];
    memset(regs3, 0, sizeof regs3);
    regs3[0] = th;
    svc_dispatch(0x09, regs3, 0);   /* StartThread */

    /* 主线程挂锁等待(0x1A):子线程 0x1B 交接 → 0x1A 返回 0,锁字 = 主线程标签 */
    r = st_svc(0x1A, 0x1001, (uint64_t)&sh.lockw, 0x2002, 0, NULL);
    ST_CHECK("ArbitrateLock 交接唤醒", r == 0 && sh.lockw == 0x2002);
    st_svc(0x1B, (uint64_t)&sh.lockw, 0, 0, 0, NULL);   /* 放锁清零 */

    /* 主线程挂条件等待(0x1C):子线程 0x1D 唤醒,经锁交接回主线程 */
    sh.lockw = 0x2002 | 0x40000000u;
    r = st_svc(0x1C, (uint64_t)&sh.lockw, (uint64_t)&sh.keyw, 0x2002,
               2000 * 1000000, NULL);
    ST_CHECK("condvar 等待被唤醒", r == 0);

    /* 主线程挂地址等待(0x34 相等则等):子线程 0x35 唤醒 */
    r = st_svc(0x34, (uint64_t)&sh.addrw, 2, 7, 2000 * 1000000, NULL);
    ST_CHECK("地址仲裁等待被唤醒", r == 0);

    /* 等线程句柄(子线程已 ExitThread) */
    uint32_t hth[1] = { (uint32_t)th };
    r = st_svc(0x18, 0, (uint64_t)(uintptr_t)hth, 1, 2000 * 1000000, &x1);
    ST_CHECK("WaitSync 线程退出即醒", r == 0);
    r = st_svc(0x16, th, 0, 0, 0, NULL);
    ST_CHECK("CloseHandle(thread)", r == 0);
    if (stk) {
        munmap(stk, 256 * 1024);
    }
#undef ST_CHECK
    snprintf(report, report_size, "svc-hle: %d/%d", pass, total);
    return pass == total ? 0 : -1;
}
