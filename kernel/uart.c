// kernel/uart.c
#include "types.h"
#include "uart.h"

#define UART_BASE 0x10000000 // QEMU virt机器的串口地址

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
    uint8_t *lcr = uart + 3;  // LCR 寄存器偏移

    // 配置 8N1 (8位数据/无校验/1停止位)
    *lcr = 0x03;
}

int puts(const char *s) {
    while(*s) uart_putc(*s++);
    uart_putc('\n');
    return 0;
}

int putchar(int c) {
    uart_putc(c);
    return c;
}