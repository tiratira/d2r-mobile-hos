/*
 * dm_guest_loader.h — 重写后游戏模块加载器(HOS,容器 v3)。
 *
 * 对应上游 dm/host/loader.c 的装载半段:镜像经 tools/hos_embed.py 编进
 * libdmhost.so(arm64 "ax" 段原地执行;镜像起始 16K 对齐,数据窗口
 * [ro_off, patch_off) 由装载器 mprotect RW 后原地重定位,代码区保持
 * .so 签名页)。x86_64 构建编 data-only 变体(纯数据,同一解析代码)。
 *
 * 容器 v3 布局(hos_embed.py 产出,小端):
 *   [镜像 patched.bin 原样,补 0 到 16K 倍数]
 *   DmGuestMeta(56B)
 *   门表(N × DmGuestGate)
 */
#ifndef DM_GUEST_LOADER_H
#define DM_GUEST_LOADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_GUEST_MAGIC 0x48523244u /* "D2RH" */
#define DM_GUEST_VERSION 3u
#define DM_GUEST_PAGE 0x4000u      /* 镜像窗口对齐(兼容 4K/16K 页) */

/* 门种类 — 与 tools/patch_nso_hos.py 的 KIND_CODE 一一对应 */
enum {
    DM_GATE_SVC = 0,
    DM_GATE_CNTPCT = 1,
    DM_GATE_CNTVCT = 2,
    DM_GATE_OBS = 3,
    DM_GATE_TPIDR_R = 4,
    DM_GATE_TPIDR_W = 5,
    DM_GATE_TPIDRRO_R = 6,
    DM_GATE_PARAM = 7
};

typedef struct {
    uint32_t magic;        /* DM_GUEST_MAGIC */
    uint32_t version;      /* DM_GUEST_VERSION */
    uint32_t module_kind;  /* 0=main 1=sdk 2=subsdk0 */
    uint32_t gate_count;
    uint64_t image_size;   /* patched.bin 字节数(未补齐,含键页与补丁区) */
    uint64_t entry_off;    /* 入口在镜像内的偏移 */
    uint64_t patch_off;    /* 补丁区起始(重写器 PATCH_OFF,16K 对齐) */
    uint64_t text_end;     /* text 段结束(代码区 [0,text_end) 保持只读+执行) */
    uint64_t ro_off;       /* ro 段起始(数据窗口 [ro_off, patch_off) 重定位+运行时可写) */
    uint64_t data_off;     /* data 段起始 */
} DmGuestMeta;

typedef struct {
    uint64_t ret_off;      /* 门块返回点在镜像内的偏移 */
    uint32_t kind;         /* DM_GATE_* */
    uint32_t arg;          /* svc:调用号 / tpidr:寄存器号 / param:(键号<<5)|寄存器 / obs:编号 */
    uint64_t orig_off;     /* 被改写的原指令位置(镜像内偏移) */
    uint64_t reserved;
} DmGuestGate;

typedef struct {
    DmGuestMeta hdr;
    const uint8_t *image;  /* 镜像首址(节起始,已随 .so 签名,不复制) */
    const DmGuestGate *gates;
    char name[16];
} DmGuestModule;

/* 双指针加载:镜像与元数据各一个符号(hos_embed 发 __dm_guest_<名>_start/_meta),
 * 加载时互验 meta 位置 == image + align(image_size, 16K)。 */
int dmGuestLoad(const void *image, const void *meta, const char *name, DmGuestModule *out);

/* 兼容旧调用:单指针 + 在 +PAGE 处试探元数据(仅占位容器用)。 */
int dmGuestLoadEmbedded(const void *blob, uint64_t blob_size, const char *name, DmGuestModule *out);

/* 元数据与门表完整性校验,不执行任何 guest 代码。 */
int dmGuestVerify(const DmGuestModule *m);

/* 按门种类查第 n 个门;返回 0 成功。 */
int dmGuestGateAt(const DmGuestModule *m, uint32_t index, DmGuestGate *out);

void dmGuestUnload(DmGuestModule *m);

/* 自检:加载占位 guest 并验证(返回 0 成功)。 */
int dmGuestSelfTest(char *report, int report_size);

/* 内嵌占位容器符号(见 dm_guest_placeholder.S)。 */
extern const uint8_t __dm_guest_placeholder_start[];

#ifdef __cplusplus
}
#endif
#endif /* DM_GUEST_LOADER_H */
