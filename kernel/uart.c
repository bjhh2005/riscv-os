// kernel/uart.c
#include "types.h"
#include "memlayout.h"
#include "defs.h"

#define UART_BASE 0x10000000 // QEMU virt机器的串口地址
#define REG(reg) ((volatile uint8 *)(UART0 + reg))

// 寄存器偏移量定义
#define RHR 0                 // Receive Holding Register (读)
#define THR 0                 // Transmit Holding Register (写)
#define IER 1                 // Interrupt Enable Register
#define FCR 2                 // FIFO Control Register
#define ISR 2                 // Interrupt Status Register
#define LCR 3                 // Line Control Register
#define LSR 5                 // Line Status Register

// 状态位定义
#define LSR_RX_READY (1<<0)   // 输入缓冲区有数据 (RHR ready)
#define LSR_TX_IDLE  (1<<5)   // 输出缓冲区为空 (THR empty)

// 寄存器偏移定义
enum {
    UART_THR = 0,  // 发送保持寄存器
    UART_RBR = 0,  // 接收缓冲寄存器  
    UART_LCR = 3,  // 线控制寄存器
    UART_LSR = 5   // 线状态寄存器
};

void uart_putc(char c) {
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    volatile uint8_t *lsr = uart + 5;  // LSR 寄存器偏移

    while ((*lsr & 0x20) == 0);  // 等待 THRE=1 (bit 5)
    *uart = c;  // 写入 THR
}



// 输出字符串
void uart_puts(char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// 可选：初始化UART（最小实现可不调用）
void uart_init() {
    volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
    volatile uint8_t *lcr = uart + 3;  // LCR 寄存器偏移

    // 配置 8N1 (8位数据/无校验/1停止位)
    *lcr = 0x03;
}

int
uartgetc(void)
{
  if(*REG(LSR) & LSR_RX_READY){
    // 读取接收寄存器
    return *REG(RHR);
  } else {
    return -1;
  }
}

int uart_getc(void){
    return uartgetc();
}

// 处理 UART 中断 (键盘输入)
void
uartintr(void)
{
  // 读取输入寄存器，直到读空
  // 否则中断信号无法消除，CPU 会一直卡在中断里
  while(1){
    int c = uartgetc();
    if(c == -1)
      break;
    
    // 如果你实现了 consoleintr(c)，就在这里调用
    // consoleintr(c); 
    
    // 暂时我们只把输入的字符回显到屏幕上，证明键盘能用
    // (在实现 shell 之前，这是个不错的测试)
    uart_putc(c); 
  }
}