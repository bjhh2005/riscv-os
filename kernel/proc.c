#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h" 

struct cpu cpus[NCPU];
struct proc proc[NPROC];
struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;
struct spinlock wait_lock;

extern void forkret(void); // 声明
extern char trampoline[];  // trampoline.S

// ----------------------------------------------------------------
// 修复后的 initcode (Test: Hello -> Fork -> Parent prints P, Child prints C)
// ----------------------------------------------------------------
uchar initcode[] = {
  // -----------------------
  // Code Section
  // -----------------------
  
  // [Start]
  // write(1, "Hello\n", 6)
  0x13, 0x05, 0x10, 0x00, // li a0, 1
  0x93, 0x05, 0x80, 0x06, // li a1, 0x68 (Data: "Hello")
  0x13, 0x06, 0x60, 0x00, // li a2, 6
  0x93, 0x08, 0x00, 0x01, // li a7, 16 (SYS_write)
  0x73, 0x00, 0x00, 0x00, // ecall

  // fork()
  0x93, 0x08, 0x10, 0x00, // li a7, 1 (SYS_fork)
  0x73, 0x00, 0x00, 0x00, // ecall

  // [关键修复] if (a0 == 0) goto child
  // 旧代码: 0x63, 0x04, 0x05, 0x00 (跳8字节，错误)
  // 新代码: 0x63, 0x04, 0x05, 0x02 (跳40字节，正确，直接跳到 child 标签)
  0x63, 0x04, 0x05, 0x02, 

  // -----------------------
  // Parent (PID 1)
  // -----------------------
  // wait(0)
  0x13, 0x05, 0x00, 0x00, // li a0, 0
  0x93, 0x08, 0x30, 0x00, // li a7, 3 (SYS_wait)
  0x73, 0x00, 0x00, 0x00, // ecall
  
  // write(1, "P\n", 2)
  0x13, 0x05, 0x10, 0x00, 
  0x93, 0x05, 0x00, 0x07, // li a1, 0x70 (Data: "P")
  0x13, 0x06, 0x20, 0x00, 
  0x93, 0x08, 0x00, 0x01, 
  0x73, 0x00, 0x00, 0x00, 
  
  // spin: j spin
  0x6f, 0x00, 0x00, 0x00, 

  // -----------------------
  // Child (PID 2)
  // -----------------------
  // child: <--- 跳转目标在这里 (Offset 0x3C)
  // write(1, "C\n", 2)
  0x13, 0x05, 0x10, 0x00, 
  0x93, 0x05, 0x40, 0x07, // li a1, 0x74 (Data: "C")
  0x13, 0x06, 0x20, 0x00, 
  0x93, 0x08, 0x00, 0x01, 
  0x73, 0x00, 0x00, 0x00, 

  // exit(0)
  0x13, 0x05, 0x00, 0x00, 
  0x93, 0x08, 0x20, 0x00, // SYS_exit
  0x73, 0x00, 0x00, 0x00, 
  0x6f, 0x00, 0x00, 0x00,

  // -----------------------
  // Data Strings (Offset 0x68)
  // -----------------------
  // 0x68: "Hello\n"
  0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x0a, 0x00, 0x00,
  // 0x70: "P\n"
  0x50, 0x0a, 0x00, 0x00,
  // 0x74: "C\n"
  0x43, 0x0a, 0x00, 0x00
};

void procinit(void) {
  struct proc *p;
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int)(p - proc));
  }
}

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

pagetable_t proc_pagetable(struct proc *p) {
  pagetable_t pagetable;
  pagetable = uvmcreate();
  if(pagetable == 0) return 0;

  if(mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }
  if(mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }
  return pagetable;
}

void proc_freepagetable(pagetable_t pagetable, uint64 sz) {
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

static struct proc* allocproc(void) {
  struct proc *p;
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

  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }

  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    kfree(p->trapframe);
    release(&p->lock);
    return 0;
  }

  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret; // 指向 forkret
  p->context.sp = p->kstack + PGSIZE;
  return p;
}

// [新增] 进程启动函数
void forkret(void) {
  printf("[Forkret] Enter forkret, pid: %d\n", myproc()->pid);
  static int first = 1;

  // 调度器切换到这里时持有锁，释放它
  release(&myproc()->lock);

  if (first) {
    // 文件系统初始化暂时留空
    printf("[Forkret] First process starting...\n");
    first = 0;
  }

  printf("[Forkret] Jump to usertrapret\n");
  // 返回用户空间
  usertrapret();
}

void userinit(void) {
  struct proc *p;
  p = allocproc();
  initproc = p;
  
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE; 
  p->trapframe->epc = 0;      
  p->trapframe->sp = PGSIZE;  

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->state = RUNNABLE;
  release(&p->lock);
}

int growproc(int n) {
  uint64 sz;
  struct proc *p = myproc();
  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) return -1;
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

int fork(void) {
  int pid;
  struct proc *np;
  struct proc *p = myproc();

  if((np = allocproc()) == 0) return -1;

  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    kfree(np->trapframe);
    proc_freepagetable(np->pagetable, np->sz);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;
  *(np->trapframe) = *(p->trapframe);
  np->trapframe->a0 = 0;

  safestrcpy(np->name, p->name, sizeof(p->name));
  pid = np->pid;
  np->parent = p;
  release(&np->lock);

  acquire(&wait_lock);
  np->state = RUNNABLE;
  release(&wait_lock);
  return pid;
}

void exit(int status) {
  struct proc *p = myproc();
  if(p == initproc) panic("init exiting");

  acquire(&wait_lock);
  struct proc *np;
  for(np = proc; np < &proc[NPROC]; np++){
    if(np->parent == p){
      np->parent = initproc;
      wakeup(initproc);
    }
  }
  wakeup(p->parent);
  p->xstate = status;
  p->state = ZOMBIE;
  release(&wait_lock);

  acquire(&p->lock);
  sched();
  panic("zombie exit");
}

int wait(uint64 addr) {
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);
  for(;;){
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        acquire(&np->lock);
        havekids = 1;
        if(np->state == ZOMBIE){
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate, sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          proc_freepagetable(np->pagetable, np->sz);
          if(np->trapframe) kfree(np->trapframe);
          np->trapframe = 0;
          np->pagetable = 0;
          np->sz = 0;
          np->pid = 0;
          np->parent = 0;
          np->name[0] = 0;
          np->killed = 0;
          np->state = UNUSED;
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    sleep(p, &wait_lock);
  }
}

void scheduler(void) {
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  for(;;){
    intr_on();
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        printf("[Sched] Switch to PID %d (%s)\n", p->pid, p->name);
        p->state = RUNNING;
        c->proc = p;
        extern void swtch(struct context*, struct context*);
        swtch(&c->context, &p->context);
        printf("[Sched] Return from PID %d\n", p->pid);
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

void sched(void) {
  int intena;
  struct proc *p = myproc();
  if(!holding(&p->lock)) panic("sched p->lock");
  if(mycpu()->noff != 1) panic("sched locks");
  if(p->state == RUNNING) panic("sched running");
  if(intr_get()) panic("sched interruptible");
  intena = mycpu()->intena;
  extern void swtch(struct context*, struct context*);
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();
  if(lk != &p->lock){
    acquire(&p->lock);
    release(lk);
  }
  p->chan = chan;
  p->state = SLEEPING;
  sched();
  p->chan = 0;
  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

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