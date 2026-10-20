/*
 * dm_guest_placeholder.S — 编译进 .so 的占位 guest 镜像。
 *
 * 上游 iOS 用 ios/tools/gen_embed.py 把 patch_nso.py 的重写产物
 * <name>.patched.bin 变成汇编编进 App;HOS 移植沿用同一策略:宿主侧
 * 脚本生成同样的 .incbin 段(见 docs/PORTING.md),此文件是带完整
 * 容器头的占位样例,供加载器自检,不含任何游戏内容。
 */
    .section .dm_guest_placeholder, "ax"
    .align 3
    .global __dm_guest_placeholder_start
    .type __dm_guest_placeholder_start, @object
__dm_guest_placeholder_start:
    /* DmGuestImageHeader (32 字节) */
    .word 0x48523244      /* magic 'D2RH' */
    .word 1               /* version */
    .word 0               /* module_kind = main */
    .word 1               /* gate_count */
    .xword 4              /* image_size */
    .xword 0              /* entry_offset */
    /* 镜像:单条 ARM64 ret 指令 */
    .word 0xd65f03c0
    /* 门表: { u32 gate_id; u64 image_offset } 自然对齐共 16 字节 */
    .word 0               /* gate_id = 0 */
    .word 0               /* 对齐填充 */
    .xword 0              /* image_offset */
    .size __dm_guest_placeholder_start, .-__dm_guest_placeholder_start
