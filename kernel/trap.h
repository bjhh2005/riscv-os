#ifndef TRAP_H
#define TRAP_H

#include "types.h"
#include "spinlock.h"

// 初始化中断子系统（创建 ticks 锁）
void trapinit(void);

// 初始化当前 CPU 的中断寄存器 (stvec, sie 等)
void trapinithart(void);

// --------------------------------------------------
// Trap 处理入口
// --------------------------------------------------

// 从内核态返回用户态
// 注意：proc.c 中的 forkret 会调用此函数
void usertrapret(void);

// 处理来自用户态的 Trap (中断/异常/系统调用)
void usertrap(void);

// 处理来自内核态的 Trap
void kerneltrap(void);

// --------------------------------------------------
// 全局变量
// --------------------------------------------------

// 系统启动以来的时钟滴答数 (用于 sys_sleep)
extern uint ticks;

// 保护 ticks 的自旋锁
extern struct spinlock tickslock;

#endif // TRAP_H