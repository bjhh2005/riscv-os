#include "types.h"
#include "riscv.h"

// SBI 调用汇编封装
// extension: 扩展ID (EID)
// function:  功能ID (FID)
static inline uint64 sbi_call(uint64 extension, uint64 function, uint64 arg0, uint64 arg1, uint64 arg2) {
    uint64 error, value;
    asm volatile (
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "mv a7, %5\n"  // EID
        "mv a6, %6\n"  // FID
        "ecall\n"
        "mv %0, a0\n"
        "mv %1, a1\n"
        : "=r" (error), "=r" (value)
        : "r" (arg0), "r" (arg1), "r" (arg2), "r" (extension), "r" (function)
        : "a0", "a1", "a2", "a6", "a7", "memory"
    );
    return error;
}

// 设置下一次时钟中断时间点
// 使用 SBI Legacy Extension (EID=0x00) 或者 Time Extension (EID=0x54494D45)
// 这里为了简单兼容性，使用 OpenSBI 推荐的 Legacy Set Timer (EID=0, FID=0)
void sbi_set_timer(uint64 stime_value) {
    // SBI Legacy Extension: Set Timer (EID=0x0)
  asm volatile("mv a0, %0" : : "r" (stime_value));
  asm volatile("li a7, 0x0"); 
  asm volatile("ecall");
}