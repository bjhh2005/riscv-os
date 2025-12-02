#include "console.h"
#include "uart.h"
#include "printf.h"


console_buffer_t console_out_buf;

// 初始化控制台（调用 UART 初始化）
void console_init(void) {
    uart_init();
    console_clear();
}

// 向控制台输出一个字符（通过 UART）
void console_putc(char c) {
    uart_putc(c);
}

// 向控制台输出一个字符串（通过 UART）
void console_puts(const char *s) {
    if (!s) return;  // 防止空指针

    // 直接调用 uart_puts，但注意参数类型可能需要转换
    // 假设 uart_puts 接受 char*，而 s 是 const char*
    uart_puts((char *)s);
}

// 清屏：发送 ANSI 清屏序列
void console_clear(void) {
    console_puts(ANSI_CLEAR_SCREEN);
}

// 设置文本颜色和背景颜色（ANSI 颜色代码）
void console_set_color(uint8_t fg_color, uint8_t bg_color) {
    // 格式: \033[<fg>;<bg>m
    char color_seq[16];
    // 使用 sprintf 格式化字符串
    sprintf(color_seq, "\033[%d;%dm", fg_color, bg_color);
    console_puts(color_seq);
}

void console_flush(void) {
    while (console_out_buf.tail != console_out_buf.head) {
        uart_putc(console_out_buf.buf[console_out_buf.tail]);
        console_out_buf.tail = (console_out_buf.tail + 1) % CONSOLE_BUF_SIZE;
    }
}