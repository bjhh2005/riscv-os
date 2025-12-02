#ifndef SPINLOCK_H
#define SPINLOCK_H

// spinlock.h
// Mutual exclusion lock.
struct spinlock {
  uint locked;       // 锁状态：0表示未上锁，1表示已上锁

  // 用于调试的字段：
  char *name;        // 锁的名称，便于调试
  struct cpu *cpu;   // 当前持有该锁的CPU的指针
  uint pcs[10];      // 获取该锁时的调用栈（程序计数器数组）
};

// 函数声明
void initlock(struct spinlock*, char*); // 初始化自旋锁
void acquire(struct spinlock*);         // 获取（加锁）自旋锁
void release(struct spinlock*);         // 释放（解锁）自旋锁
int holding(struct spinlock*);          // 检查当前CPU是否持有该锁（主要用于调试）

// 以下是某些实现中与中断控制相关的函数，用于防止死锁
void push_off(void); // 禁用中断，并记录之前的中断状态（可嵌套调用）
void pop_off(void);  // 恢复之前的中断状态（与 push_off 配对使用）

#endif