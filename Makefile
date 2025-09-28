# 编译器设置
CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
CFLAGS = -mcmodel=medany -fno-pic -Wall -Iinclude -nostdlib -ffreestanding -fno-builtin
LDFLAGS = -T kernel/kernel.ld -nostdlib

# 目标文件列表
OBJS = \
    kernel/entry.o \
    kernel/main.o \
    kernel/uart.o \
    kernel/printf.o \
    kernel/console.o \
    kernel/string.o \
	kernel/sleep.o

# 默认目标：编译并生成 kernel
all: kernel/kernel

# 编译规则：.S 和 .c 文件 -> .o 文件
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 链接规则：.o 文件 -> kernel
kernel/kernel: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# 运行 QEMU
run: kernel/kernel
	qemu-system-riscv64 -machine virt -kernel kernel/kernel -nographic

# 清理生成的文件
clean:
	rm -f $(OBJS) kernel/kernel

.PHONY: all run clean