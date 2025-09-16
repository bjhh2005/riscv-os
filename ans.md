# 答案
## 实验1：RISC-V引导与裸机启动
### 任务1：理解xv6启动流程
#### 为什么第一条指令是设置栈指针
1. ​C 代码依赖栈运行​
2. 栈的设置是其他初始化的前提​

#### stack0在哪里定义
在 `start.c` 中，stack0 是一个数组，通常定义为 char stack0[4096 * NCPU]，其中 NCPU 是 CPU 核心数。每个核心（hart）有自己的 4096 字节栈空间。
```c
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];
```

#### 为什么要清零BSS段
`kernel.ld`
```ld
.bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }
```
1. ​C 语言标准要求​：未初始化的全局/静态变量必须初始化为 0
2. ​安全性和确定性​：
    - 清零确保内核启动时所有全局状态已知
    - 防止未初始化内存泄露敏感信息（安全漏洞）

#### 如何从汇编跳转到C函数
call start

#### `ENTRY(_entry_)`的作用是什么
明确指定程序执行的入口点，即操作系统内核启动时，CPU 首先执行的代码位置。
#### 为什么代码段要放在`0x80000000`
​硬件强制要求​, 软件生态约定.

### 任务2: 设计最小启动流程


### 任务3：实现启动汇编代码
1. 创建`kernel/enrty.s`
2. 设置入口点和栈指针
3. 清零BSS段
4. 跳转到C主函数

### 任务4：编写链接脚本
1. 确定起始地址
2. 组织代码段、数据段、BSS段
3. 定义必要的符号供C代码使用
```bash
riscv64-unknown-elf-objdump -h kernel.elf
riscv64-unknown-elf-nm kernel.elf | grep -E "(start|end|text)"
```

### 任务5: 实现串口驱动
参考xv6种的`uart.c`，实现最小功能

先实现`void uart_putc(char c)`，再实现`void uart_puts(char* s)`.

### 任务6：完成C主函数

```bash
# 编译所有文件
riscv64-unknown-elf-gcc -mcmodel=medany -fno-pic -Wall -c kernel/entry.S -o kernel/entry.o
riscv64-unknown-elf-gcc -mcmodel=medany -fno-pic -Wall -c kernel/main.c -o kernel/main.o
riscv64-unknown-elf-gcc -mcmodel=medany -fno-pic -Wall -c kernel/uart.c -o kernel/uart.o
# 链接
riscv64-unknown-elf-ld -T kernel/kernel.ld -o kernel/kernel \
    kernel/entry.o kernel/main.o kernel/uart.o
```

```bash
qemu-system-riscv64 -machine virt -kernel kernel/kernel -nographic
```