# kernel/entry.S
# 修复版：适应 OpenSBI 的 S-Mode 启动

.section .text.entry
.global _start

_start:
    # 1. 设置栈指针
    la sp, stack_top

    # 2. [调试] 打印 'K' (Kernel Loaded)
    # 这证明 OpenSBI 成功跳转到了这里
    li t0, 0x10000000
    li t1, 'K'
    sb t1, 0(t0)

    # 3. 清零 BSS
    la a0, _bss_start
    la a1, _bss_end
    call clear_bss

    # 4. 跳转到 C 主函数
    # [关键修改] 改回 call main
    # 因为 OpenSBI 已经把我们放在了 S-Mode，start.c (M-Mode代码) 会导致崩溃
    call main

    # 5. 死循环
spin:
    j spin

# --- BSS 清零 ---
clear_bss:
    bge a0, a1, clear_done
    sb zero, 0(a0)
    addi a0, a0, 1
    j clear_bss
clear_done:
    ret

# --- 栈空间 ---
.section .bss
    .align 4
    .global stack_top
    .space 4096 * 16
stack_top: