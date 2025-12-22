#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

//
// PLIC (Platform Level Interrupt Controller) 处理外部中断
// (如 UART 串口输入, VirtIO 磁盘完成信号)
//

void
plicinit(void)
{
  // 设置中断优先级
  // UART0 = 1, VIRTIO0 = 1
  // 地址计算依赖 memlayout.h 中的 PLIC 宏
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

void
plicinithart(void)
{
  int hart = cpuid();
  
  // 设置当前 CPU (Hart) 的 S-mode 阈值
  // 只有优先级 > 0 的中断才会触发
  *(uint32*)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
}

// 询问 PLIC 哪个设备发生了中断
int
plic_claim(void)
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// 告诉 PLIC 我们已经处理完该中断了
void
plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}