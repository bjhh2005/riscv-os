#ifndef __MEMLAYOUT_H__
#define __MEMLAYOUT_H__

// 引入 riscv.h 以使用 PGSIZE 等常量
#include "riscv.h" 

// --- 1. 物理内存布局 ---
#define KERNBASE 0x80200000L   // 内核起始物理地址
#define PHYSTOP  0x88000000L   // 物理内存结束 (128MB)

// --- 2. 设备地址 ---
#define UART0    0x10000000L
#define VIRTIO0  0x10001000L
#define PLIC     0x0c000000L
#define CLINT    0x2000000L

// --- 3. 内核栈布局 ---
#define KERNEL_STACK_PAGES 4
#define KERNEL_STACK_SIZE (KERNEL_STACK_PAGES * PGSIZE)
// 注意：KERNEL_STACK_TOP 通常是虚拟地址空间中的位置，这里仅作定义

// --- 4. 用户空间布局 ---
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)
#define USER_STACK_TOP TRAPFRAME

// --- 5. 链接器符号声明 ---
extern char end[];   // 内核代码结束位置
extern char etext[]; // 代码段结束位置

#endif