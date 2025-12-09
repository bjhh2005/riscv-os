#include "printf.h"
#include "console.h"
#include "uart.h"
#include "kalloc.h"
#include "vm.h"
#include "riscv.h"
#include <stdint.h>

// --- 外部函数声明 ---
extern void trapinit(void);
extern void trapinithart(void);

// --- 辅助函数：忙等待 ---
void spin_delay(int count) {
    volatile int i = 0;
    for (i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

// --- 辅助函数：斐波那契测试 ---
int stress_fib(int n) {
    if (n <= 1) return n;
    if (n > 10 && n % 2 == 0) spin_delay(100); 
    return stress_fib(n - 1) + stress_fib(n - 2);
}

// --- 辅助函数：触发异常 ---
void test_exception_trigger() {
    printf("\n[DANGER] Triggering Synchronous Exception (Store Page Fault)...\n");
    *(volatile int*)0x0 = 0xdeadbeef; // 写入 0 地址
}

/* --- 系统主入口 --- */
void main(void) {
    // 1. 基础硬件初始化 (UART)
    uart_init();
    console_init();
    
    printf("\n=== RISC-V OS: Interrupt & Trap Lab Test Suite ===\n");
    
    // 2. 内存初始化 (只做一次！)
    printf("[Init] Physical Memory...\n");
    kinit();
    
    printf("[Init] Virtual Memory...\n");
    kvminit();
    kvminithart();

    // 3. 中断初始化
    printf("[Init] Interrupt System...\n");
    trapinit();      // 软件表注册
    trapinithart();  // 硬件寄存器设置
    
    printf("[Ready] System initialized. Enabling Global Interrupts...\n");

    // 4. 开启中断
    intr_on();

    // 5. 开始测试 (斐波那契并发测试)
    printf("\n=== TEST 1: Concurrency & Context Save/Restore ===\n");
    printf("Calculating Fibonacci(25). Expect 'timer: tick' lines to appear below:\n");

    volatile int n = 25;
    int result = 0;
    
    for(int round = 1; round <= 3; round++) {
        printf("[Main] Round %d start calculation...\n", round);
        
        // 耗时计算，应当被时钟中断打断
        result = stress_fib(n);
        
        if (result == 75025) {
             printf("[Main] Round %d Result: %d (Correct)\n", round, result);
        } else {
             printf("[Main] Round %d Result: %d (WRONG!)\n", round, result);
             while(1); 
        }
        
        spin_delay(5000000); // 暂停一会，观察输出
    }

    // 6. 异常测试
    printf("\n=== TEST 2: Exception Handling ===\n");
    printf("Executing exception test in 3 seconds...\n");
    spin_delay(20000000);
    
    test_exception_trigger();

    // 死循环
    while(1);
}