#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_write(void)
{
  int fd;
  uint64 p;
  int n;

  if(argint(0, &fd) < 0 || argaddr(1, &p) < 0 || argint(2, &n) < 0)
    return -1;

  if(fd == 1 || fd == 2){
    struct proc *pr = myproc();
    for(int i = 0; i < n; i++){
      char c;
      if(copyin(pr->pagetable, &c, p + i, 1) < 0)
        return -1;
      console_putc(c);
    }
    return n;
  }
  return -1;
}