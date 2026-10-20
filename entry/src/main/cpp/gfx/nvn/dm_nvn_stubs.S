/*
 * dm_nvn_stubs.S — NVN 未实现入口的静态 thunk 表(ARM64)。
 *
 * 与上游 nvn_thunks.S 同思路,但只做「记名记录」:每个入口 8 字节
 *   movz x9,#k ; b dm_nvn_unimpl_entry
 * 调用进来时 x9 = 槽号,把 x0-x8 原样交给 C 侧 dm_nvn_unimpl_c,
 * 名字来自引导时登记的槽名表;首次调用打一行 hilog,返回 0。
 *
 * 不生成运行时代码(与 .so 签名段同一约束),录制层落实后逐槽换成真实实现。
 */
    .text

    .globl dm_nvn_thunks
    .p2align 4
dm_nvn_thunks:
    .set k, 0
    .rept 512                   // 与 dm_nvn.c 的 NVN_TABLE_MAX 一致
    movz x9, #k
    b dm_nvn_unimpl_entry
    .set k, k + 1
    .endr

    .globl dm_nvn_unimpl_entry
    .p2align 2
dm_nvn_unimpl_entry:
    stp x29, x30, [sp, #-96]!
    mov x29, sp
    stp x0, x1, [sp, #16]
    stp x2, x3, [sp, #32]
    stp x4, x5, [sp, #48]
    stp x6, x7, [sp, #64]
    str x8, [sp, #80]
    mov x0, x9
    add x1, sp, #16
    bl dm_nvn_unimpl_c
    ldp x29, x30, [sp], #96
    ret
