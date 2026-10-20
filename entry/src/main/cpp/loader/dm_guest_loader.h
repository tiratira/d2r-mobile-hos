/*
 * dm_guest_loader.h — 重写后游戏模块加载器(HOS 骨架)。
 *
 * 对应上游 dm/host/loader.c:把 patch_nso.py 重写产物映射进内存、
 * 按门表注册 host gate、做跨模块重定位、设置段权限。
 *
 * 上游在 iOS 上不允许运行时生成可执行页,因此用 tools/gen_embed.py
 * 把重写镜像编进 App 的签名代码段;OHOS 同样要求代码页签名,移植
 * 采用相同策略:镜像经汇编 .incbin 进入带 "ax" 段的 libdmhost.so。
 *
 * 容器格式(DmGuestImageHeader)由宿主侧打包脚本从 patch_nso.py 的
 * 产物 (<name>.patched.bin / .gates.tsv) 生成,见 docs/PORTING.md。
 */
#ifndef DM_GUEST_LOADER_H
#define DM_GUEST_LOADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_GUEST_MAGIC 0x48523244u /* "D2RH" */

typedef struct {
    uint32_t magic;        /* DM_GUEST_MAGIC */
    uint32_t version;      /* 容器版本,当前 1 */
    uint32_t module_kind;  /* 0=main 1=sdk 2=subsdk0 */
    uint32_t gate_count;   /* 门表条数 */
    uint64_t image_size;   /* 重写镜像字节数 */
    uint64_t entry_offset; /* nnMain 在镜像内的偏移 */
} DmGuestImageHeader;

typedef struct {
    uint32_t gate_id;
    uint64_t image_offset; /* gate 跳板相对镜像基址的偏移 */
} DmGuestGate;

typedef struct {
    DmGuestImageHeader hdr;
    const uint8_t *base;   /* 已映射基址(PROT_READ|PROT_EXEC) */
    uint64_t mapped_size;
    const DmGuestGate *gates;
    char name[16];
} DmGuestModule;

/*
 * 从文件加载(开发/回归用)。注意:OHOS 对运行时映射 RX 匿名页有
 * 代码签名限制,真机路径是 dmGuestLoadEmbedded;文件路径保留给
 * 模拟器与验证环境。
 */
int dmGuestLoad(const char *path, const char *name, DmGuestModule *out);

/* 从编进 .so 的内嵌镜像加载(正式路径,等价上游 gen_embed.py 方案)。 */
int dmGuestLoadEmbedded(const void *blob, const char *name, DmGuestModule *out);

/* 验证头并按门表检查镜像完整性,不执行任何 guest 代码。 */
int dmGuestVerify(const DmGuestModule *m);

/* 查询门偏移;未注册返回 -1。 */
int64_t dmGuestGateOffset(const DmGuestModule *m, uint32_t gate_id);

void dmGuestUnload(DmGuestModule *m);

/* 自检:加载编译进 .so 的占位 guest 段并验证,返回 0 成功。 */
int dmGuestSelfTest(char *report, int report_size);

/* 内嵌占位镜像符号(见 dm_guest_placeholder.S)。 */
extern const uint8_t __dm_guest_placeholder_start[];

#ifdef __cplusplus
}
#endif
#endif /* DM_GUEST_LOADER_H */
