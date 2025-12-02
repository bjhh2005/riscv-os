// kernel/uart.c
#include "types.h"
#include "uart.h"

#define UART_BASE 0x10000000 // QEMU virt机器的串口地址

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
