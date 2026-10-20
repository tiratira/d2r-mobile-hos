/*
 * dm_guest_loader.h — 重写后游戏模块加载器(HOS,容器 v2)。
 *
 * 对应上游 dm/host/loader.c:把重写产物登记进内存、按门表注册宿主门。
 * OHOS 与 iOS 一样要求代码页签名,镜像经 tools/hos_embed.py 编进
 * libdmhost.so 的 "ax" 段(等价上游 gen_embed.py 策略),加载器只登记
 * 基址与门,不再复制代码。
 *
 * 容器 v2 布局(hos_embed.py 产出,小端):
 *   DmGuestImageHeader(32B)+ patched.bin 原样 + 门表(N × DmGuestGate)
 */
#ifndef DM_GUEST_LOADER_H
#define DM_GUEST_LOADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_GUEST_MAGIC 0x48523244u /* "D2RH" */
#define DM_GUEST_VERSION 2u

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
    uint64_t image_size;   /* patched.bin 字节数(含键页与补丁区) */
    uint64_t entry_offset; /* 入口在镜像内的偏移 */
} DmGuestImageHeader;

typedef struct {
    uint64_t ret_off;      /* 门块返回点(blr 后一条)在镜像内的偏移 */
    uint32_t kind;         /* DM_GATE_* */
    uint32_t arg;          /* svc:调用号 / tpidr:寄存器号 / param:(键号<<5)|寄存器 / obs:编号 */
    uint64_t orig_off;     /* 被改写的原指令位置(镜像内偏移) */
    uint64_t reserved;
} DmGuestGate;

typedef struct {
    DmGuestImageHeader hdr;
    const uint8_t *blob;   /* 容器起始(已随 .so 签名,不复制) */
    const uint8_t *image;  /* blob + 32:patched.bin 首址 */
    const DmGuestGate *gates;
    char name[16];
} DmGuestModule;

/* 从编进 .so 的内嵌容器加载(正式路径)。 */
int dmGuestLoadEmbedded(const void *blob, uint64_t blob_size, const char *name, DmGuestModule *out);

/* 头与门表完整性校验,不执行任何 guest 代码。 */
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
