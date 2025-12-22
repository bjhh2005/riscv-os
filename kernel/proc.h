#ifndef __PROC_H__
#define __PROC_H__

#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "vm.h" // 引用你的 vm.h 以获取 pagetable_t 定义
#include"param.h"
// 进程调度上下文 (Swtch Context)
// 仅保存 Callee-saved 寄存器
struct kcontext {
  uint64 ra;
  uint64 sp;
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

// CPU 状态
struct cpu {
  struct proc *proc;          // 当前运行的进程
  struct kcontext context;    // 调度器的上下文
  int noff;                   // 关中断深度
  int intena;                 // 关中断前的状态
};

// 进程状态
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// 进程控制块
struct proc {
  struct spinlock lock;

  // p->lock 保护以下字段:
  enum procstate state;        
  void *chan;                  // 休眠通道
  int killed;                  
  int xstate;                  
  int pid;                     

  // 进程私有字段:
  uint64 kstack;               // 内核栈地址
  uint64 sz;                   // 内存大小
  pagetable_t pagetable;       // 用户页表
  struct trapframe *trapframe; // 指向 Trapframe 页 (结构体定义在 trap.h 或此处前置声明)
  struct kcontext context;     // switch 在这里切换
  char name[16];               
  struct proc *parent;         
};

// 导出全局变量
extern struct cpu cpus[NCPU];
extern struct proc proc[NPROC];

// 导出函数接口
void procinit(void);
struct proc* allocproc(void);
void scheduler(void);
void sched(void);
void yield(void);
void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);
int cpuid(void);
struct cpu* mycpu(void);
struct proc* myproc(void);

#endif