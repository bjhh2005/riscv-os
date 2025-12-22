#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "kalloc.h"
#include "vm.h"
#include "printf.h"
#include "string.h"

struct cpu cpus[NCPU];
struct proc proc[NPROC];
struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;
struct spinlock wait_lock;

// 声明外部汇编函数
extern void swtch(struct kcontext *old, struct kcontext *new);
extern void forkret(void);

// --- 辅助函数 ---

int cpuid() {
  int id = r_tp();
  return id;
}

struct cpu* mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

struct proc* myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid() {
  int pid;
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);
  return pid;
}

// --- 进程管理核心 ---

void procinit(void) {
  struct proc *p;
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  
  // 初始化所有进程槽位
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = 0;
  }
}

// 分配一个新进程
struct proc* allocproc(void) {
  struct proc *p;

  // 1. 寻找空闲槽位
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // 2. 分配 Trapframe 页
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }
  memset(p->trapframe, 0, PGSIZE);

  // 3. 创建空的用户页表
  p->pagetable = uvmcreate();
  if(p->pagetable == 0){
    kfree(p->trapframe);
    release(&p->lock);
    return 0;
  }

  // 4. 分配内核栈
  if((p->kstack = (uint64)kalloc()) == 0){
    destroy_pagetable(p->pagetable); 
    kfree(p->trapframe);
    release(&p->lock);
    return 0;
  }
  memset((void*)p->kstack, 0, PGSIZE);

  // 5. 初始化调度上下文
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  // 【注意】函数返回时，p->lock 依然被持有！
  // 调用者必须负责 release(&p->lock)
  return p;
}

// --- 调度器 ---

// CPU 调度主循环 (每个 CPU 都会运行这个函数)
void scheduler(void) {
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // 必须开启中断，否则无法响应时钟中断，无法抢占
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // 找到可运行进程，切换过去
        p->state = RUNNING;
        c->proc = p;
        
        // [关键] 上下文切换
        // 从当前调度器上下文 -> 进程上下文
        swtch(&c->context, &p->context);

        // 进程让出 CPU 后回到这里
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// 让出 CPU (进入调度器)
void sched(void) {
  int intena;
  struct proc *p = myproc();

  // 严谨的锁检查
  if(!holding(&p->lock)) panic("sched p->lock");
  if(mycpu()->noff != 1) panic("sched locks");
  if(p->state == RUNNING) panic("sched running");
  if(intr_get()) panic("sched interruptible");

  intena = mycpu()->intena; // 保存中断状态
  
  // 切换回调度器上下文
  swtch(&p->context, &mycpu()->context);
  
  mycpu()->intena = intena; // 恢复中断状态
}

// 主动放弃 CPU
void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// --- 同步原语 ---

// 睡眠：释放 lk，挂起当前进程，等待 wakeup
void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();
  
  if(p == 0) panic("sleep");

  // 如果持有的不是 p->lock，需要交换锁
  // (保证 p->state 修改的原子性)
  if(lk != &p->lock){
    acquire(&p->lock);
    release(lk);
  }

  p->chan = chan;
  p->state = SLEEPING;

  sched(); // 切换进程

  // 醒来后清理
  p->chan = 0;

  // 恢复原来的锁
  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

// 唤醒：找到所有在 chan 上睡眠的进程，设为 RUNNABLE
void wakeup(void *chan) {
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// forkret: 新进程第一次被调度时的入口
// 实际上如果我们在 main.c 中劫持了 ra，这个函数可能不会被执行
void forkret(void) {
  release(&myproc()->lock);
  printf("forkret: process %d started\n", myproc()->pid);
  while(1); 
}