#include "types.h"
#include "riscv.h"
#include "printf.h"
#include "uart.h"

extern void kinit(void);
extern void kvminit(void);
extern void kvminithart(void);
extern void trapinit(void);
extern void trapinithart(void);

void spin_delay(int count) {
    volatile int i;
    for (i = 0; i < count; i++) asm volatile("nop");
}

// 斐波那契测试 (放在最前面测)
int stress_fib(int n) {
    if (n <= 1) return n;
    if (n > 10 && n % 5 == 0) spin_delay(100); 
    return stress_fib(n - 1) + stress_fib(n - 2);
}

// 模拟系统调用
int sys_write_test(int fd, char *buf, int len) {
    int ret;
    asm volatile(
        "li a7, %1\n" // 【修复】使用 li 加载立即数
        "mv a0, %2\n" 
        "mv a1, %3\n"
        "mv a2, %4\n"
        "ecall\n"     
        "mv %0, a0\n" 
        : "=r"(ret) 
        : "i"(1), "r"(fd), "r"(buf), "r"(len)
        : "a0", "a1", "a2", "a7"
    );
    return ret;
}

void trigger_page_fault() {
    printf(RED "[DANGER]" RESET " Writing to Null Pointer (0x0)...\n");
    *(volatile int*)0x0 = 0xDEADBEEF;
}

void trigger_soft_intr() {
    printf(YELLOW "[TEST]" RESET " Triggering Software Interrupt...\n");
    unsigned long mask = 2; 
    asm volatile("csrs sip, %0" : : "r"(mask));
}

void main(void) {
    uart_init();
    printf("\n" CYAN "=== RISC-V OS: Final Robustness Test ===" RESET "\n");

    kinit();
    kvminit();
    kvminithart();
    trapinit();     
    trapinithart(); 
    intr_on();
    printf(GREEN "[READY]" RESET " System Initialized.\n");

    // ---------------------------------------------------------
    // TEST 1: 时钟并发测试 (最先执行，防止堆栈被污染)
    // ---------------------------------------------------------
    printf("\n" YELLOW "=== TEST 1: Concurrency (Fibonacci) ===" RESET "\n");
    volatile int n = 35; 
    printf("Calculating Fib(%d)...\n", n);
    
    int res = stress_fib(n);
    // Fib(35) = 9227465
    if (res == 9227465) {
        printf(GREEN "[PASS]" RESET " Fib Result: %d (Correct)\n", res);
    } else {
        printf(RED "[FAIL]" RESET " Fib Result: %d (Wrong!)\n", res);
    }

    // ---------------------------------------------------------
    // TEST 2: 软件中断测试
    // ---------------------------------------------------------
    printf("\n" YELLOW "=== TEST 2: Software Interrupt ===" RESET "\n");
    trigger_soft_intr();
    spin_delay(500000); 

    // ---------------------------------------------------------
    // TEST 3: 系统调用测试
    // ---------------------------------------------------------
    printf("\n" YELLOW "=== TEST 3: System Call (Ecall) ===" RESET "\n");
    sys_write_test(1, "Hello Syscall!", 14); 

    // ---------------------------------------------------------
    // TEST 4: 异常恢复测试
    // ---------------------------------------------------------
    printf("\n" YELLOW "=== TEST 4: Fault Recovery ===" RESET "\n");
    
    // A: 非法指令 (系统应跳过)
    printf(RED "[DANGER]" RESET " Executing Illegal Instruction...\n");
    asm volatile(".word 0x00000000");
    printf(GREEN "[SURVIVED]" RESET " Skipped bad instruction!\n");

    // B: 缺页异常 (系统应跳过)
    trigger_page_fault();
    printf(GREEN "[SURVIVED]" RESET " Skipped bad write!\n");

    // ---------------------------------------------------------
    // TEST 5: 键盘交互 (死循环)
    // ---------------------------------------------------------
    printf("\n" YELLOW "=== TEST 5: Interactive Keyboard ===" RESET "\n");
    printf("System is quiet now. Type anything to test UART!\n");

    while(1) {
        spin_delay(1000000);
    }
}