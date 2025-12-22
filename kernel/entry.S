# kernel/entry.S

.section .text.entry
.global _start

_start:
    # 1. 初始化 tp (hartid)
    mv tp, a0

    # 2. 设置栈
    la sp, stack_base
    li t0, 4096
    addi t1, a0, 1
    mul t0, t0, t1
    add sp, sp, t0

    # 3. 只有 Hart 0 清空 BSS
    bnez tp, skip_bss_clear
    la a0, _bss_start
    la a1, _bss_end
    bge a0, a1, skip_bss_clear

clear_bss_loop:
    sd zero, 0(a0)
    addi a0, a0, 8
    blt a0, a1, clear_bss_loop

skip_bss_clear:
    # ==============================================
    # 4. [关键修复] 跳转到 start，而不是 main
    # start 函数会负责切换到 S-Mode 并跳到 main
    # ==============================================
    call start

spin:
    j spin

# 数据段定义保持不变
.section .bss
.align 4
.global stack_base
.global _bss_start
.global _bss_end

stack_base:
    .space 4096 * 8