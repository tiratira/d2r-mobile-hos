/*
 * entry_hos.S — 上游 dm/host/entry.S 的 OHOS/ELF 移植。
 *
 * 0) unimpl_stubs:未实现导入的记名桩,第 k 个 = movz x9,#k; b unimpl_entry
 *    (静态代码;OHOS 与 iOS 同样不许运行时生成代码)。
 * 1) unimpl_entry:桩入口,x9 = 桩号;保存 x0-x8 调 unimpl_c(idx, regs)。
 * 2) dm_gate:patch_nso_hos.py 的门桩以 `bl dm_gate` 进入;进来时 lr = 门块
 *    返回点(gate_dispatch 据此查 gates.tsv),保存 x0-x30 + q0-q31 + NZCV,
 *    调 gate_dispatch(ret, regs),回写后返回。
 *    与 Apple 版的差异:HOS 上 x18 是客体自己的 TCB(不改写),必须原样
 *    保存并恢复 —— Apple 版跳过恢复是因为内核会改它。
 * 3) dm_thread_enter(entry, arg, sp):客体线程起点,切客体栈调入口。
 */
    .text

    .globl unimpl_stubs
    .p2align 3
unimpl_stubs:
    .set k, 0
    .rept 4096                // 与 hle_core.c 的 DM_MAX_STUB 一致
    movz x9, #k
    b unimpl_entry
    .set k, k + 1
    .endr

    .globl unimpl_entry
    .p2align 2
unimpl_entry:
    stp x29, x30, [sp, #-96]!
    mov x29, sp
    stp x0, x1, [sp, #16]
    stp x2, x3, [sp, #32]
    stp x4, x5, [sp, #48]
    stp x6, x7, [sp, #64]
    str x8, [sp, #80]
    mov x0, x9
    add x1, sp, #16
    bl unimpl_c
    ldp x29, x30, [sp], #96
    ret

    .globl dm_gate
    .p2align 2
dm_gate:
    sub sp, sp, #800
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    stp q0, q1, [sp, #256]
    stp q2, q3, [sp, #288]
    stp q4, q5, [sp, #320]
    stp q6, q7, [sp, #352]
    stp q8, q9, [sp, #384]
    stp q10, q11, [sp, #416]
    stp q12, q13, [sp, #448]
    stp q14, q15, [sp, #480]
    stp q16, q17, [sp, #512]
    stp q18, q19, [sp, #544]
    stp q20, q21, [sp, #576]
    stp q22, q23, [sp, #608]
    stp q24, q25, [sp, #640]
    stp q26, q27, [sp, #672]
    stp q28, q29, [sp, #704]
    stp q30, q31, [sp, #736]
    mrs x9, nzcv
    str x9, [sp, #768]
    mrs x9, fpsr
    str x9, [sp, #776]
    mrs x9, fpcr
    str x9, [sp, #784]
    mov x0, x30              // 返回点 = 门标识
    mov x1, sp               // regs
    bl gate_dispatch
    ldr x9, [sp, #784]
    msr fpcr, x9
    ldr x9, [sp, #776]
    msr fpsr, x9
    ldr x9, [sp, #768]
    msr nzcv, x9
    ldp q0, q1, [sp, #256]
    ldp q2, q3, [sp, #288]
    ldp q4, q5, [sp, #320]
    ldp q6, q7, [sp, #352]
    ldp q8, q9, [sp, #384]
    ldp q10, q11, [sp, #416]
    ldp q12, q13, [sp, #448]
    ldp q14, q15, [sp, #480]
    ldp q16, q17, [sp, #512]
    ldp q18, q19, [sp, #544]
    ldp q20, q21, [sp, #576]
    ldp q22, q23, [sp, #608]
    ldp q24, q25, [sp, #640]
    ldp q26, q27, [sp, #672]
    ldp q28, q29, [sp, #704]
    ldp q30, q31, [sp, #736]
    ldp x0, x1, [sp, #0]
    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x19, [sp, #144]  // HOS:x18 = 客体 TCB,必须恢复
    ldp x20, x21, [sp, #160]
    ldp x22, x23, [sp, #176]
    ldp x24, x25, [sp, #192]
    ldp x26, x27, [sp, #208]
    ldp x28, x29, [sp, #224]
    ldr x30, [sp, #240]
    add sp, sp, #800
    ret

    .globl dm_thread_enter
    .p2align 2
dm_thread_enter:
    mov sp, x2
    mov x29, #0
    mov x16, x0
    mov x0, x1
    mov x18, xzr          // Horizon 新线程 x18 初值 = 0(客体自写自读的 TCB)
    adr x30, 1f
    br x16
1:  bl dm_thread_returned
    brk #0
