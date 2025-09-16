.section .text
.global _entry

_entry:
    # 调试检查点1: 程序开始执行
    li t0, 0x10000000   # UART基地址
    li t1, 'S'          # Start标记
    sb t1, 0(t0)        # 输出'S'

    # 设置栈指针
    la sp, _stack_end
    li a0, 4096
    add sp, sp, a0

    # 调试检查点2: 栈设置完成
    li t1, 'P'          # Stack Pointer ready
    sb t1, 0(t0)        # 输出'P'

    # 清零BSS段
    la a0, __bss_start
    la a1, __bss_end
clear_bss:
    beq a0, a1, clear_done
    sw zero, (a0)
    addi a0, a0, 4
    j clear_bss
clear_done:

    # 跳转到C主函数
    call main

spin:
    j spin