# trampoline.S
# 只有 trampoline 页被映射在用户页表和内核页表的同一个虚拟地址
# 因此我们在切换页表时必须在这段代码中执行

.section .trampsec
.globl trampoline
trampoline:
.align 12
.globl uservec
uservec:    
        #
        # trap.c 中的 usertrap() 将 stvec 指向这里
        # 当用户态发生中断/异常时，硬件跳转到这里
        #
        # sscratch 指向 trapframe 的地址 (在 usertrapret 中设置)
        # 交换 a0 和 sscratch，现在 a0 是 trapframe 指针，sscratch 是原来的 a0
        csrrw a0, sscratch, a0

        # 保存用户寄存器到 trapframe
        sd ra, 40(a0)
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)

        # 保存原来的 a0 (现在在 sscratch 中)
        csrr t0, sscratch
        sd t0, 112(a0)

        # ----------------------------------------------
        # 切换到内核环境
        # ----------------------------------------------

        # 从 trapframe 加载内核栈指针、内核 hartid、内核 trap 处理函数
        ld sp, 8(a0)
        ld tp, 32(a0)
        ld t0, 16(a0)  # t0 = usertrap
        ld t1, 0(a0)   # t1 = kernel_satp

        # 切换页表！(将 satp 设置为内核页表)
        csrw satp, t1
        sfence.vma zero, zero

        # 跳转到 usertrap()
        jr t0

.globl userret
userret:
        #
        # usertrapret() 调用这里，从内核返回用户态
        # a0: TRAPFRAME 的虚拟地址 (在 TRAMPOLINE 页中)
        # a1: 用户页表的 satp 值
        #

        # 切换页表：使用用户页表
        csrw satp, a1
        sfence.vma zero, zero

        # 现在 a0 仍然指向 trapframe，因为 trampoline 映射在同样的位置
        # 将 trapframe 地址存入 sscratch，供下次 trap 使用
        csrw sscratch, a0

        # 从 trapframe 恢复用户寄存器
        ld ra, 40(a0)
        ld sp, 48(a0)
        ld gp, 56(a0)
        ld tp, 64(a0)
        ld t0, 72(a0)
        ld t1, 80(a0)
        ld t2, 88(a0)
        ld s0, 96(a0)
        ld s1, 104(a0)
        ld a1, 120(a0)
        ld a2, 128(a0)
        ld a3, 136(a0)
        ld a4, 144(a0)
        ld a5, 152(a0)
        ld a6, 160(a0)
        ld a7, 168(a0)
        ld s2, 176(a0)
        ld s3, 184(a0)
        ld s4, 192(a0)
        ld s5, 200(a0)
        ld s6, 208(a0)
        ld s7, 216(a0)
        ld s8, 224(a0)
        ld s9, 232(a0)
        ld s10, 240(a0)
        ld s11, 248(a0)
        ld t3, 256(a0)
        ld t4, 264(a0)
        ld t5, 272(a0)
        ld t6, 280(a0)
        
        # 最后恢复 a0
        ld a0, 112(a0)
        
        # 返回用户态 (sepc 和 sstatus 已经在 usertrapret 中设置好了)
        sret