#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "types.h"

struct spinlock {
  uint locked;       // 是否被锁
  char *name;        // 锁名字
  int cpu_id;        // 持有该锁的 CPU ID (调试用)
};

void initlock(struct spinlock *lk, char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif