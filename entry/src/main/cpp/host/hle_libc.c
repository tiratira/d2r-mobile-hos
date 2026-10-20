/*
 * hle_libc.c — 未由三模块互相定义、也不属于 nn 服务的少数导入:
 *   getentropy                                 客体 libc 的随机源
 *   nnosGetDefaultUserExceptionStackForApplication   SDk 运行时的异常栈查询(未用)
 *   __rel_dyn_start/end、__rel_plt_start/end   链接器提供的重定位范围标记,
 *     客体不真正使用;给一对首尾相等的合法地址,避免落记名桩。
 *
 * 其余 libc/libc++ 导入由 sdk 模块自带定义,跨模块解析即可(见 loader_hos.c
 * 的导入覆盖报告)。
 */
#include "host/dm.h"
#include "hal/dm_platform.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* ---------- getentropy(上游 hle_os.c 同接口;HOS 用内核随机源) ---------- */
HLE("getentropy", int, h_getentropy, void *buf, size_t n)
{
    if (n > 256) {
        errno = EIO;
        return -1;
    }
    static int fd = -1;
    if (fd < 0) {
        fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    }
    size_t got = 0;
    while (got < n) {
        ssize_t r = fd >= 0 ? read(fd, (uint8_t *)buf + got, n - got) : -1;
        if (r <= 0) {
            /* 兜底:用计数器搅一下,至少不返回全 0 */
            for (size_t i = got; i < n; i++) {
                ((uint8_t *)buf)[i] = (uint8_t)(dmPlatCounterRead() >> (8 * (i & 7)));
            }
            return 0;
        }
        got += (size_t)r;
    }
    return 0;
}

/* ---------- SDK 异常栈查询:宿主没有 Horizon 的异常栈 ---------- */
HLE("nnosGetDefaultUserExceptionStackForApplication", void *, h_nnos_exc_stack, void)
{
    return NULL;
}

/* ---------- 链接器重定位范围标记(数据符号) ---------- */
static uint64_t g_rel_dyn[2];
static uint64_t g_rel_plt[2];

__attribute__((used, section("swhle"), aligned(8)))
static const hle_ent hle_ent_rel_dyn_start = {"__rel_dyn_start", &g_rel_dyn[0]};
__attribute__((used, section("swhle"), aligned(8)))
static const hle_ent hle_ent_rel_dyn_end = {"__rel_dyn_end", &g_rel_dyn[1]};
__attribute__((used, section("swhle"), aligned(8)))
static const hle_ent hle_ent_rel_plt_start = {"__rel_plt_start", &g_rel_plt[0]};
__attribute__((used, section("swhle"), aligned(8)))
static const hle_ent hle_ent_rel_plt_end = {"__rel_plt_end", &g_rel_plt[1]};
