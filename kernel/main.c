#include "printf.h"
#include "console.h"
#include "uart.h"
#include "sleep.h"
#include <stdint.h>

/* 测试辅助宏 */
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_RESET   "\033[0m"

/* 断言测试工具 */
static void assert(int condition, const char *msg) {
    if (!condition) {
        printf(ANSI_COLOR_RED "[FAIL] %s" ANSI_COLOR_RESET "\n", msg);
        while(1); // 测试失败挂起
    }
}

/* 测试通过标记 */
static void test_pass(const char *name) {
    printf(ANSI_COLOR_GREEN "[PASS] %s" ANSI_COLOR_RESET "\n", name);
}

/* BSS段清零测试 */
static void test_bss_clear(void) {
    static int bss_var;
    static char bss_buf[128];
    
    assert(bss_var == 0, "BSS variable not zeroed");
    for (int i = 0; i < 128; i++) {
        assert(bss_buf[i] == 0, "BSS buffer not zeroed");
    }
    test_pass("BSS clearance test");
}

/* 栈操作测试 */
static void test_stack_operation(void) {
    int stack_var = 0xCAFEBABE;
    assert(stack_var == 0xCAFEBABE, "Stack corruption detected");
    test_pass("Stack operation test");
}

/* printf基础格式测试 */
static void test_printf_formats(void) {
    console_clear();
    printf("=== printf Format Testing ===\n");
    
    // 测试十进制输出
    printf("Decimal output: %d\n", 12345);
    
    // 测试十六进制输出  
    printf("Hexadecimal: 0x%x\n", 0xDEADBEEF);
    
    // 测试字符串输出
    printf("String format: %s\n", "TestString");
    
    // 测试边界值
    printf("INT32_MAX: %d\n", INT32_MAX);
    printf("INT32_MIN: %d\n", INT32_MIN);
    
    test_pass("Basic format testing");
}

/* 错误格式处理测试 */
static void test_error_handling(void) {
    printf("\n=== Error Handling Tests ===\n");
    
    // 测试无效格式符
    printf("Invalid formats: %z %q % \n");
    
    test_pass("Error handling cases");
}

/* ANSI控制功能测试 */
static void test_ansi_features(void) {
    printf("\n=== ANSI Control Tests ===\n");
    
    // 测试颜色输出
    console_set_color(FG_RED, BG_BLACK);
    printf("Red text ");
    console_set_color(FG_GREEN, BG_BLACK);
    printf("Green text ");
    console_set_color(FG_WHITE, BG_BLACK);
    printf("\n");
    
    // 测试清屏功能
    printf("Screen will clear in 3 seconds...\n");
    sleep(3000);
    console_clear();
    printf("Screen cleared successfully!\n");
    
    test_pass("ANSI control sequences");
}

/* 性能压力测试 */
static void test_performance(void) {
    printf("\n=== Performance Stress Test ===\n");
    
    for (int i = 0; i < 100; i++) {
        printf("Line %03d: ", i);
        for (int j = 0; j < 5; j++) {
            printf("%d ", i*j);
        }
        printf("\n");
    }
    
    test_pass("High-frequency output");
}

/* 实验二主测试入口 */
void lab2(void) {
    printf("=== Lab 2: Console Subsystem Test ===\n");
    
    // 第一阶段：基础硬件验证
    test_bss_clear();
    test_stack_operation();
    
    // 第二阶段：核心功能测试
    test_printf_formats();
    test_error_handling();
    test_ansi_features();
    
    // 第三阶段：系统压力测试
    test_performance();
    
    printf("\n=== Lab 2 Tests Completed ===\n");
}

/* 系统主入口 */
void main(void) {
    // 硬件初始化
    uart_init();
    console_init();
    
    // 执行实验二测试
    lab2();
    
    // // 主循环
    // while(1) {

    // }
}