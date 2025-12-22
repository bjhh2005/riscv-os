#ifndef PROC_H
#define PROC_H

#include "param.h"
#include "riscv.h"
#include "types.h"
#include "spinlock.h"

// 前向声明，防止因缺少 fs.h/file.h 报错
struct file;
struct inode;

// 用于内核上下文切换的寄存器保存 (switch.S)
struct context {
  uint64 ra;
  uint64 sp;
  // callee-saved registers
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

// 每个 CPU 的状态
struct cpu {
  struct proc *proc;          // 当前在该 CPU 运行的进程
  struct context context;     // 用于 swtch() 切换到调度器
  int noff;                   // push_off() 嵌套深度
  int intena;                 // push_off() 之前的中断状态
};

extern struct cpu cpus[NCPU];

// -----------------------------------------------------------
// Trapframe: 用户态中断时保存寄存器的结构体
// 注意：这个结构体的布局必须和 trampoline.S 中的偏移量一致
// -----------------------------------------------------------
struct trapframe {
  /* 0 */ uint64 kernel_satp;   // 内核页表
  /* 8 */ uint64 kernel_sp;     // 进程的内核栈顶
  /* 16 */ uint64 kernel_trap;   // usertrap() 函数地址
  /* 24 */ uint64 epc;           // 保存的用户程序计数器 (PC)
  /* 32 */ uint64 kernel_hartid; // 保存的内核 tp
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

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 进程控制块 (PCB)
struct proc {
  struct spinlock lock;

  // 使用以下字段需要持有 p->lock:
  enum procstate state;        // 进程状态
  void *chan;                  // 如果非空，表示正在该通道上睡眠
  int killed;                  // 如果非空，表示已被杀掉
  int xstate;                  // 退出状态码 (供父进程 wait 读取)
  int pid;                     // 进程 ID

  // 使用以下字段需要持有 wait_lock:
  struct proc *parent;         // 父进程

  // 这里的字段是进程私有的，不需要持有锁:
  uint64 kstack;               // 内核栈虚拟地址
  uint64 sz;                   // 进程内存大小 (字节)
  pagetable_t pagetable;       // 用户页表
  struct trapframe *trapframe; // 映射到用户空间的 trapframe 页
  struct context context;      // swtch() 在此切换运行进程
  struct file *ofile[NOFILE];  // 打开的文件 (暂时不用，但保留结构)
  struct inode *cwd;           // 当前目录 (暂时不用)
  char name[16];               // 进程名 (调试用)
};

#endif