#ifndef DEFS_H
#define DEFS_H

#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "console.h"
#include "proc.h"
#include "vm.h"

// kalloc.c
void* kalloc(void);
void            kfree(void *);
void            kinit(void);
void            kref_inc(void *); // COW Step 2
void* kalloc_pages(int);

// vm.c
void            kvminit(void);
void            kvminithart(void);
uint64          kvmpa(uint64);
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);
int             mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uchar *, uint);
uint64          uvmalloc(pagetable_t, uint64, uint64);
uint64          uvmdealloc(pagetable_t, uint64, uint64);
int             uvmcopy(pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
void            uvmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          walkaddr(pagetable_t, uint64);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
int             cow_alloc(pagetable_t, uint64); // COW Step 2

// proc.c
int             cpuid(void);
void            exit(int);
int             fork(void);
int             growproc(int);
void            proc_mapstacks(pagetable_t);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint64);
int             kill(int);
struct cpu* mycpu(void);
struct cpu* getmycpu(void);
struct proc* myproc(void);
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             wait(uint64);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);

// trap.c
extern uint     ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            usertrapret(void);

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// printf.c
void            printfinit(void);
void            printf(char*, ...);
void            sprintf(char*, char*, ...);
void            panic(char*) __attribute__((noreturn));



// syscall.c
void            syscall(void);
int             argint(int, int*);
int             argaddr(int, uint64*);

// 外部 SBI 接口 (通常在 sbi.c 或类似地方，或者是汇编接口)
void sbi_set_timer(uint64 stime_value);

// plic.c
void            plicinit(void);
void            plicinithart(void);
int             plic_claim(void);
void            plic_complete(int);

// uart.c
void            uart_init(void);
void            uartintr(void);
void            uart_putc(int);
void            uart_puts(char*);
void            uartputc_sync(int);
int             uart_getc(void);
#endif