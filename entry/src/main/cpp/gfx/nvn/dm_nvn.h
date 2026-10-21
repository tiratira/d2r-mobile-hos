/*
 * dm_nvn.h — NVN 引导接口层骨架(上游 dm/gfx/nvn/hle_nvn.c 的角色)。
 */
#ifndef DM_NVN_H
#define DM_NVN_H

#include <stdint.h>
#include <stddef.h>

/*
 * 上游:游戏 main 模块只导入一个 NVN 入口 nvnBootstrapLoader,宿主以
 * HLE_FORCE 强制接管,SDK 内 NVN 驱动一条指令都不执行;hle_nvn.c 把游戏
 * 的 NVN 调用录成命令,nvn_mtl.m 回放为 Metal。
 *
 * HOS:同一结构,回放端是 Vulkan(gfx/dm_vk)。本骨架先提供函数表分发与
 * 记名桩(每个 NVN 入口第一次调用记一行 hilog),供录制层平移时逐个落实。
 */
typedef void (*nvn_func_t)(void);

#ifdef __cplusplus
extern "C" {
#endif

/* 宿主侧 nvnBootstrapLoader 实现:按名字取 NVN 函数表指针。 */
uint64_t dmNvnBootstrapLoader(const char *name);

/* 登记宿主已实现的 NVN 入口(bootstrap/proc 分发时优先返回真实函数)。 */
void dmNvnRegister(const char *name, nvn_func_t fn);

/* 启动时登记最小真实入口(当前:nvnDeviceGetProcAddress)。 */
void dmNvnInit(void);

/* 已解析的宿主函数地址 -> NVN 名字(崩溃现场 GOT 槽反查用)。 */
const char *dmNvnNameForAddr(uint64_t addr);

/* 纹理存储 → 线性(M2 资源层;回放上传用) */
int dmNvnTextureLinearize(const void *tex, unsigned level, unsigned layer,
                          const void *src_storage, void *dst, size_t dst_pitch);

/* 已登记入口数 / 被调用过的入口数(观测骨架是否被走到)。 */
uint32_t dmNvnEntryCount(void);
uint32_t dmNvnCalledCount(void);

/* 自检:引导 + 取表 + 调一个入口,返回报告。 */
const char *dmNvnSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif /* DM_NVN_H */

