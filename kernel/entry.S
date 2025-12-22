# kernel/entry.S

.section .text.entry
.global _start

_start:
    # ==============================================
    # 1. [关键修复] 初始化 tp 寄存器
    # OpenSBI 跳转过来时，a0 保存了 hartid。
    # xv6 约定 tp 寄存器保存 hartid，用于 cpuid() 函数。
    # 必须最先执行，因为后面的代码可能会修改 a0。
    # ==============================================
    mv tp, a0

    # ==============================================
    # 2. 设置栈指针 (Stack Pointer)
    # 为每个 CPU 分配独立的 4KB 栈，防止冲突。
    # 算法: sp = stack_top + (hartid + 1) * 4096
    # ==============================================
    la sp, stack_base      # 加载栈底地址
    li t0, 4096            # 栈大小
    addi t1, a0, 1         # t1 = hartid + 1
    mul t0, t0, t1         # t0 = 4096 * (hartid + 1)
    add sp, sp, t0         # sp = 对应 CPU 的栈顶

    # ==============================================
    # 3. 清零 BSS 段 (仅由主核 Hart 0 执行)
    # 其它核心跳过此步骤
    # ==============================================
    bnez tp, skip_bss_clear

    la a0, _bss_start      # BSS 起始地址
    la a1, _bss_end        # BSS 结束地址
    bge a0, a1, skip_bss_clear

clear_bss_loop:
    sd zero, 0(a0)         # 清零 8 字节
    addi a0, a0, 8
    blt a0, a1, clear_bss_loop

skip_bss_clear:
    # ==============================================
    # 4. 跳转到 C 语言主函数
    # ==============================================
    call main

    # 5. 如果 main 返回 (理论上不应该)，进入死循环
spin:
    j spin

# ==============================================
# 数据段定义
# ==============================================
.section .bss
.align 4
.global stack_base
.global _bss_start
.global _bss_end

# 分配足够 8 个 CPU 使用的栈空间 (8 * 4096)
stack_base:
    .space 4096 * 8 
    # 栈是向下生长的，所以 sp 会指向这里的某个高地址位置