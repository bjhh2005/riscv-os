#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// =================================================================
// 补全缺失的参数获取函数
// =================================================================

// 内部辅助函数：获取第 n 个系统调用参数 (保存在寄存器 a0-a5 中)
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch(n){
  case 0: return p->trapframe->a0;
  case 1: return p->trapframe->a1;
  case 2: return p->trapframe->a2;
  case 3: return p->trapframe->a3;
  case 4: return p->trapframe->a4;
  case 5: return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// 获取第 n 个 int 类型的参数
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// 获取第 n 个指针类型的参数 (地址)
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// =================================================================
// 以下是你原有的代码
// =================================================================

// 声明具体的系统调用函数 (在 sysproc.c 中实现)
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_getpid(void);
extern uint64 sys_write(void);

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

// 函数指针数组
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_getpid]  sys_getpid,
[SYS_write]   sys_write,
};

// 系统调用入口
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  // 系统调用号存放在 a7 寄存器中
  num = p->trapframe->a7;

  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // 执行系统调用，返回值存入 a0
    p->trapframe->a0 = syscalls[num]();
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}