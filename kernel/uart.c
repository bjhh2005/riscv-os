// kernel/uart.c
#include "types.h"
#include "uart.h"

#define UART_BASE 0x10000000 // QEMU virt机器的串口地址

// 输出单个字符
void uart_putc(char c) {
    volatile char *uart = (volatile char *)UART_BASE;
    *uart = c;
}

// 输出字符串
void uart_puts(char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// 可选：初始化UART（最小实现可不调用）
void uart_init() {
    // QEMU虚拟串口默认已初始化，此处留空
}