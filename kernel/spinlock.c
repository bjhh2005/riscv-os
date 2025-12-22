#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// 获取锁
// 类似于 pthread_mutex_lock
void
acquire(struct spinlock *lk)
{
  push_off(); // 禁止中断，防止死锁

  if(holding(lk))
    panic("acquire");

  // 原子交换指令 (Atomic Swap)
  // __sync_lock_test_and_set 是 GCC 内置函数，对应 RISC-V 的 amoswap
  // 如果 lk->locked 原来是 0，则写入 1 并返回 0 -> 获得锁
  // 如果 lk->locked 原来是 1，则写入 1 并返回 1 -> 自旋等待
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;

  // 内存屏障，保证临界区代码不会被重排到获得锁之前
  __sync_synchronize();

  // 记录持有锁的 CPU，方便调试
  lk->cpu = mycpu();
}

// 释放锁
// 类似于 pthread_mutex_unlock
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // 内存屏障，保证临界区代码不会被重排到释放锁之后
  __sync_synchronize();

  // 原子释放锁
  // __sync_lock_release 将 lk->locked 置为 0
  __sync_lock_release(&lk->locked);

  pop_off(); // 恢复中断
}

// 检查当前 CPU 是否持有该锁
int
holding(struct spinlock *lk)
{
  int r;
  push_off();
  r = (lk->locked && lk->cpu == mycpu());
  pop_off();
  return r;
}

// -----------------------------------------------------------
// 中断管理 (用于防止死锁)
// -----------------------------------------------------------

// 关中断，并增加嵌套计数
void
push_off(void)
{
  int old = intr_get();

  intr_off();
  
  // mycpu() 需要 defs.h 或 proc.h 支持
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  
  mycpu()->noff += 1;
}

// 减小嵌套计数，如果计数归零则恢复中断
void
pop_off(void)
{
  struct cpu *c = mycpu();
  
  if(intr_get())
    panic("pop_off - interruptible");
  
  if(c->noff < 1)
    panic("pop_off");
    
  c->noff -= 1;
  
  if(c->noff == 0 && c->intena)
    intr_on();
}