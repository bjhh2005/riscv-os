#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[]; // trampoline.S
extern void uservec(void); // trampoline.S
extern void userret(void); // trampoline.S
extern void kernelvec(void); // kernelvec.S

// [新增] 前向声明，解决 "undeclared" 错误
void usertrap(void);
int devintr(void);

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
}

// -------------------------------------------------------------------
// 返回用户空间
// -------------------------------------------------------------------
void
usertrapret(void)
{
  struct proc *p = myproc();

  intr_off();

  w_sepc(p->trapframe->epc);

  uint64 x = r_sstatus();
  x &= ~SSTATUS_SPP; 
  x |= SSTATUS_SPIE; 
  w_sstatus(x);
  

  // [修复] 指针运算错误：强制转换为 uint64
  // 计算 uservec 在 trampoline 页中的偏移量
  uint64 trampoline_uservec = TRAMPOLINE + ((uint64)uservec - (uint64)trampoline);
  w_stvec(trampoline_uservec);

  p->trapframe->kernel_satp = r_satp();         
  p->trapframe->kernel_sp = p->kstack + PGSIZE; 
  p->trapframe->kernel_trap = (uint64)usertrap; 
  p->trapframe->kernel_hartid = r_tp();         

  uint64 satp = MAKE_SATP(p->pagetable);

  // [修复] 指针运算错误：强制转换为 uint64
  // 计算 userret 在 trampoline 页中的偏移量
  uint64 fn = TRAMPOLINE + ((uint64)userret - (uint64)trampoline);
  // [调试] 打印跳转地址
  printf("[Trap] Return to user, calling fn: %p, satp: %p\n", fn, satp);
  
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// -------------------------------------------------------------------
// 用户态 Trap 处理
// -------------------------------------------------------------------
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  p->trapframe->epc = r_sepc();

  uint64 scause = r_scause();

  if(scause == 8){
    // System Call
    if(p->killed)
      exit(-1);
    p->trapframe->epc += 4;
    intr_on();
    syscall(); // 需要在 defs.h 中声明
  } 
  else if((which_dev = devintr()) != 0){
    // Device Interrupt
  } 
  else if(scause == 15){
    // Store Page Fault (COW)
    uint64 va = r_stval();
    if(cow_alloc(p->pagetable, va) < 0){
       p->killed = 1;
    }
  } 
  else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  if(which_dev == 2)
    yield();

  usertrapret();
}

// -------------------------------------------------------------------
// 内核态 Trap 处理
// -------------------------------------------------------------------
void
kerneltrap(void)
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause 0x%lx\n", scause);
    printf("sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// -------------------------------------------------------------------
// 设备中断分发
// -------------------------------------------------------------------
int
devintr(void)
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) && (scause & 0xff) == 9){
    // PLIC 外部中断 (这里通常需要 plic.c 支持)
    // int irq = plic_claim();
    // if(irq) plic_complete(irq);
    return 1;
  } else if(scause == 0x8000000000000005L){
    // 时钟中断
    if(cpuid() == 0){
      clockintr();
    }
    
    // 设置下一次中断，防止一直触发
    // 注意：sbi_set_timer 需要在 defs.h 声明
    sbi_set_timer(r_time() + 100000); 
    
    // 清除 SIP 中的 SSIP 位
    // 注意：r_sip 需要在 riscv.h 中实现
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}