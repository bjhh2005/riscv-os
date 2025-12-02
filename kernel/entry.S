# entry.S
# 使用 .text.entry 段名，配合 linker script 确保它放在最前面
.section .text.entry
.global _start

_start:
    # 1. 调试输出 'S' (Start)
    # 向 UART 串口发送字符，表明机器已上电并开始执行指令
    li t0, 0x10000000
    li t1, 'S'
    sb t1, 0(t0)

    # 2. 设置栈指针
    # 注意：RISC-V 中栈是向下生长的，所以 SP 指向高地址
    la sp, stack_top
    
    # 调试输出 'P' (Pointer/Stack set) - 栈设置完成
    li t1, 'P'
    sb t1, 0(t0)

    # 3. 清零 BSS 段
    la a0, _bss_start       # 从 linker script 获取 bss 起始地址
    la a1, _bss_end         # 从 linker script 获取 bss 结束地址
    call clear_bss          # 调用清零函数

    # 调试输出 'B' (BSS cleared) - BSS 清零完成
    li t1, 'B'
    sb t1, 0(t0)

    # 4. 跳转到 C 语言主函数
    call main

    # 5. 如果 main 返回（不应该发生），输出 'E' 并死循环
    li t1, 'E'
    sb t1, 0(t0)
spin:
    j spin

# --- BSS 清零函数 ---
clear_bss:
    bge a0, a1, clear_done  # 如果 current >= end，结束
clear_loop:
    sb zero, 0(a0)          # 将当前地址字节清零
    addi a0, a0, 1          # 指针 + 1
    blt a0, a1, clear_loop  # 如果 current < end，继续循环
clear_done:
    ret

# --- 栈空间定义 ---
.section .bss
    .align 4                # 16字节对齐
    .global stack_top
    .space 4096 * 16        # 分配 64KB 栈空间
stack_top:                  # 栈顶标签（位于空间末尾）