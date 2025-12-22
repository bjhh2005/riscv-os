#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[], kernelvec[];

// 引用 proc.c 中定义的 MLFQ 时间片数组
extern int time_slices[NPRIO];

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
}

void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // 保存用户程序计数器
  p->trapframe->epc = r_sepc();
  
  uint64 scause = r_scause();

  if(scause == 8){
    // System Call
    if(p->killed)
      exit(-1);

    // sepc 指向 ecall 指令，需要 +4 指向下一条指令
    p->trapframe->epc += 4;

    intr_on();
    syscall();
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

  // 如果是时钟中断，检查时间片并让出 CPU
  if(which_dev == 2) {
      // MLFQ: 增加消耗统计
      p->ticks_consumed++;
      
      // MLFQ: 检查是否用完当前优先级的时间片
      if (p->ticks_consumed >= time_slices[p->priority]) {
          // 如果不是最低优先级，则降级
          if (p->priority < NPRIO - 1) {
              p->priority++;
          }
          // 重置时间片计数（无论是降级还是保持在最低级）
          p->ticks_consumed = 0;
      }
      
      yield();
  }

  usertrapret();
}

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

  // 内核态的时钟中断处理
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING) {
    struct proc *p = myproc();
    
    // MLFQ: 同样在内核态统计时间消耗
    p->ticks_consumed++;
    
    if (p->ticks_consumed >= time_slices[p->priority]) {
        if (p->priority < NPRIO - 1) {
            p->priority++;
        }
        p->ticks_consumed = 0;
    }
    
    yield();
  }

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

// 检查中断源
int
devintr()
{
  uint64 scause = r_scause();

  // 处理软件中断 (由时钟中断触发)
  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    
    // 这是一个 S-mode 外部中断 (PLIC)
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      // virtio_disk_intr();
      printf("virtio intr\n");
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // 软件中断 (来自 machine-mode timer interrupt)
    
    if(cpuid() == 0){
      clockintr();
    }
    
    // 清除 SIP 中的 SSIP 位，停止中断
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

void
usertrapret(void)
{
  struct proc *p = myproc();

  // 关闭中断，因为我们要更新 trapframe 和寄存器
  intr_off();

  // 发送中断和异常到 uservec (trampoline.S)
  // 因为我们即将返回用户态
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // 设置 trapframe 中的内核信息，供下次 trap 使用
  p->trapframe->kernel_satp = r_satp();         // 内核页表
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // 进程的内核栈顶
  p->trapframe->kernel_trap = (uint64)usertrap; // 内核 trap 处理函数
  p->trapframe->kernel_hartid = r_tp();         // 当前 CPU ID

  // 设置 S Status 寄存器
  // SSTATUS_SPP (Supervisor Previous Privilege) set to User mode (0)
  // SSTATUS_SPIE (Supervisor Previous Interrupt Enable) set to Enable (1)
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // 清除 SPP (设为 User)
  x |= SSTATUS_SPIE; // 设置 SPIE (允许中断)
  w_sstatus(x);

  // 设置 SEPC 为保存的用户 PC
  w_sepc(p->trapframe->epc);

  // 告诉 trampoline 用户页表在哪里
  uint64 satp = MAKE_SATP(p->pagetable);

  // 跳转到 trampoline.S 中的 userret 函数
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64, uint64))trampoline_userret)(TRAPFRAME, satp);
}