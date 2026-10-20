/*
 * dm_guest_placeholder.S — 编译进 .so 的占位 guest 容器(v2)。
 *
 * 正式产物由 tools/patch_nso_hos.py + tools/hos_embed.py 从你的游戏 NSO 生成
 * (容器 v2,与 dm_guest_loader.h 对齐)。本文件是手工等价样例:镜像体只有
 * 一个 HOS 门桩(stp; bl dm_gate; ldp; ret),门表登记一个 tpidr_r 门
 * (ret_off=8, arg=x0)。宿主自检会实际穿过 dm_gate 执行它,验证
 * 线程环境/门分派/寄存器回写全链路。不含任何游戏内容。
 */
    .section .dm_guest_placeholder, "ax"
    .p2align 12
    .globl __dm_guest_placeholder_start
    .type __dm_guest_placeholder_start, @object
__dm_guest_placeholder_start:
    /* DmGuestImageHeader v2 (32B) */
    .word 0x48523244      /* magic 'D2RH' */
    .word 2               /* version */
    .word 0               /* module_kind = main */
    .word 1               /* gate_count */
    .xword 16             /* image_size */
    .xword 0              /* entry_offset = 0(入口即门桩) */
    /* 镜像:HOS 门桩 + ret(等价 patch_nso_hos.py 的 site_gate 产物) */
    stp x29, x30, [sp, #-16]!
    bl dm_gate            /* ← hos_embed.py 的 gatebl 替换点,手写版直接写指令 */
    ldp x29, x30, [sp], #16
    ret
    /* 门表 @ 32+16 = 48(已 8 字节对齐) */
    .globl __dm_guest_placeholder_gates
__dm_guest_placeholder_gates:
    .xword 8              /* ret_off:bl 后一条(ldp) */
    .word 4               /* kind = DM_GATE_TPIDR_R */
    .word 0               /* arg = x0 */
    .xword 0              /* orig_off */
    .xword 0              /* reserved */
    .size __dm_guest_placeholder_start, .-__dm_guest_placeholder_start
