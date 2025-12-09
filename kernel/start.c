// kernel/start.c
#include "types.h"
#include "riscv.h"

// 声明 main 函数，start 执行完后会跳过去
void main();

// 声明外部定义的 timerinit (如果后续你决定恢复独立 timer.c)，
// 但在你当前的 trap.c 架构下，时钟是在 trap.c/main.c 中初始化的，
// 所以这里只需要负责模式切换。

/*
 * start() 运行在机器模式 (Machine Mode)
 * 它的任务是配置 M 模式的特权级，设置中断委托，
 * 然后降级到监管者模式 (Supervisor Mode) 并跳转到 main()。
 */
void start() {
  // 1. 设置 M Status 寄存器
  // 我们希望执行 mret 后，CPU 进入 Supervisor Mode (MPP=01)
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK; // 清除 MPP 位
  x |= MSTATUS_MPP_S;     // 设置为 S 模式
  w_mstatus(x);

  // 2. 设置 M Exception Program Counter (MEPC)
  // 当执行 mret 时，PC 会跳转到 mepc 指向的地址
  // 我们将其指向 main 函数的入口
  w_mepc((uint64)main);

  // 3. 禁用分页 (SATP = 0)
  // 在进入 main 之前，我们先关闭页表，直接使用物理地址
  // main 函数里的 kvminithart 会再次开启它
  w_satp(0);

  // 4. 中断委托 (Delegation)
  // 默认情况下，所有中断和异常都在 M 模式处理。
  // 我们需要将它们委托给 S 模式，这样 OS 内核 (trap.c) 才能收到中断。
  
  // 委托所有异常 (Exceptions)
  w_medeleg(0xffff);
  
  // 委托所有中断 (Interrupts)
  w_mideleg(0xffff);
  
  // 5. 开启中断使能
  // 设置 sie 寄存器，允许 S 模式接受外部中断、时钟中断、软件中断
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // 6. 配置物理内存保护 (PMP)
  // 允许 S 模式访问所有物理内存地址
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // 7. 执行 mret
  // 这一步会发生：
  // - 特权级从 M 变为 S
  // - PC 变为 main 的地址
  // - 开启中断响应 (如果配置了的话)
  asm volatile("mret");
}