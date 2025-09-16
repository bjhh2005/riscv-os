// kernel/main.c
#include "uart.h"

// 全局变量测试BSS段清零
static int bss_test;
static char test_buffer[64];

void main() {
    // 检查点1: 证明已进入main函数
    uart_puts("M"); // Main entered
    
    // 检查点2: 验证BSS段是否清零
    if (bss_test == 0 && test_buffer[0] == 0) {
        uart_puts("B"); // BSS cleared correctly
    } else {
        uart_puts("X"); // BSS clear failed
    }
    
    // 检查点3: 测试栈功能
    int stack_var = 0x1234;
    if (stack_var == 0x1234) {
        uart_puts("T"); // Stack test passed
    } else {
        uart_puts("Y"); // Stack test failed
    }
    
    // 检查点4: 完整消息输出
    // uart_puts("\nMyOS Boot Success!\n");
    // uart_puts("Debug Sequence: S->P->M->B->T\n");
    
    // 主循环
    while (1) {
        // 可以添加更多功能
    }
}