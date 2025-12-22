#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "printf.h"  // 引用你的 printf.h
#include "proc.h"    // 引用 proc.h 获取 mycpu()

void initlock(struct spinlock *lk, char *name) {
  lk->name = name;
  lk->locked = 0;
  lk->cpu_id = -1;
}

void acquire(struct spinlock *lk) {
  push_off(); 
  if(holding(lk))
    panic("acquire");

  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;
  __sync_synchronize();
  
  // 记录持有锁的 CPU，方便调试
  // 注意：这里需要 proc.h 里的 cpuid()
  lk->cpu_id = cpuid();
}

void release(struct spinlock *lk) {
  if(!holding(lk))
    panic("release");

  lk->cpu_id = -1;
  __sync_synchronize();
  __sync_lock_release(&lk->locked);
  pop_off();
}

int holding(struct spinlock *lk) {
  return lk->locked && lk->cpu_id == cpuid();
}

void push_off(void) {
  int old = intr_get();
  intr_off();
  struct cpu *c = mycpu();
  if(c->noff == 0)
    c->intena = old;
  c->noff += 1;
}

void pop_off(void) {
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    intr_on();
}