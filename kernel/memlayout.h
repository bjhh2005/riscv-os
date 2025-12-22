#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

// 引入 riscv.h 以使用 PGSIZE 等常量
#include "riscv.h" 

// --- 1. 物理内存布局 ---
#define KERNBASE 0x80000000L   // 内核起始物理地址
#define PHYSTOP (KERNBASE + 128*1024*1024) // 物理内存结束地址 (128MB)

// --- 2. 设备地址 ---
#define UART0    0x10000000L
#define VIRTIO0  0x10001000L
#define PLIC     0x0c000000L
#define CLINT    0x2000000L

// --- 3. 内核栈布局 ---
#define KERNEL_STACK_PAGES 4
#define KERNEL_STACK_SIZE (KERNEL_STACK_PAGES * PGSIZE)
// 注意：KERNEL_STACK_TOP 通常是虚拟地址空间中的位置，这里仅作定义
// 映射内核栈的位置：在 TRAMPOLINE 下方
// 每个进程分配 2页 (1页栈 + 1页保护页)


// --- 4. 用户空间布局 ---
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)
#define USER_STACK_TOP TRAPFRAME
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)
// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0_IRQ 1

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)


// --- 5. 链接器符号声明 ---
extern char end[];   // 内核代码结束位置
extern char etext[]; // 代码段结束位置

#endif