#ifndef _TYPES_H_
#define _TYPES_H_

// 基本无符号类型
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

// 明确位宽的类型（C99风格）
typedef unsigned char   uint8_t;    // 8位无符号整数
typedef signed char     int8_t;     // 8位有符号整数
typedef unsigned short  uint16_t;   // 16位无符号整数
typedef signed short    int16_t;    // 16位有符号整数
typedef unsigned int    uint32_t;   // 32位无符号整数
typedef signed int      int32_t;    // 32位有符号整数
typedef unsigned long   uint64_t;   // 64位无符号整数
typedef signed long     int64_t;    // 64位有符号整数

// 兼容旧定义
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// 指针和地址类型
typedef uint64_t uintptr_t;  // 足够存放指针值的无符号整数
typedef uint64_t pde_t;      // 页目录项类型（假设64位系统）

typedef uint32_t size_t;
typedef int32_t ssize_t;


#endif // _TYPES_H_

// 
// 虚拟地址空间布局 (Sv39, 512GB地址空间)

// 0xFFFFFFFFFFFFFFFF (MAXVA)
// ├── TRAMPOLINE    (蹦床页面)     - 系统调用/异常入口点
// ├── TRAPFRAME     (陷阱帧)       - 保存用户态上下文
// ├── 内核栈保护区
// ├── 内核栈        (每个进程独立)
// ├── 内核数据段    (可读写)
// ├── 内核代码段    (可读可执行)
// └── 0x0000000000000000
//     ├── 用户栈     (向下增长)
//     ├── 用户堆     (向上增长)
//     ├── 用户数据段
//     └── 用户代码段