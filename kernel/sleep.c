#include <stdint.h>

// 基于 CPU 空循环的简单延时（单位：毫秒）
void sleep(uint32_t ms) {
    volatile uint32_t cycles = ms * 500000;
    while (cycles--);
}