#ifndef __PROC_H__
#define __PROC_H__

#include "types.h"
#include "riscv.h"

// 保存内核上下文切换时的寄存器 (switch.S 使用)
// 只需要保存 Callee-saved 寄存器
struct context {
  uint64 ra;
  uint64 sp;

  // Callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// 发生中断/异常时，保存用户态所有寄存器的结构
// 存放在 TRAMPOLINE 下面的 TRAPFRAME 页中
struct trapframe {
  /* 0 */ uint64 kernel_satp;   // kernel page table
  /* 8 */ uint64 kernel_sp;     // top of process's kernel stack
  /* 16 */ uint64 kernel_trap;   // usertrap()
  /* 24 */ uint64 epc;           // saved user program counter
  /* 32 */ uint64 kernel_hartid; // saved kernel tp
  
  /* 40 */ uint64 ra;
  /* 48 */ uint64 sp;
  /* 56 */ uint64 gp;
  /* 64 */ uint64 tp;
  /* 72 */ uint64 t0;
  /* 80 */ uint64 t1;
  /* 88 */ uint64 t2;
  /* 96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

// 简单的进程状态枚举
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 简化的 proc 结构体，为下个实验占位
struct proc {
  enum procstate state;        // 进程状态
  int pid;                     // 进程ID
  struct context context;      // 内核上下文 (switch.S)
  struct trapframe *trapframe; // 用户态中断帧 (映射到固定物理地址)
  
  uint64 kstack;               // 内核栈虚拟地址
  uint64 sz;                   // 进程内存大小
  pagetable_t pagetable;       // 用户页表
};

#endif