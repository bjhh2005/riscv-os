# 工具链设置
CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

# 编译选项
# -Ikernel: 防止头文件在 kernel 目录下时找不到
# -mcmodel=medany: 允许内核代码在任意地址运行
# -g: 生成调试信息，方便 gdb 调试
CFLAGS = -mcmodel=medany -fno-pic -Wall -Werror -O2 -g
CFLAGS += -Iinclude -Ikernel -nostdlib -ffreestanding -fno-builtin
CFLAGS += -MD -MP # 自动生成依赖关系

# 链接选项
LDFLAGS = -T kernel/kernel.ld -nostdlib -static

# 源文件列表
# wildcard 自动查找 kernel 目录下的所有 .c 和 .S 文件
SRCS = $(wildcard kernel/*.c)
OBJS = $(SRCS:.c=.o)
ASM_SRCS = $(wildcard kernel/*.S)
ASM_OBJS = $(ASM_SRCS:.S=.o)

# 最终目标文件
TARGET = kernel/kernel

# 依赖文件 (.d)
DEPS = $(OBJS:.o=.d)

# 伪目标
.PHONY: all run debug disasm clean info

# 默认目标：编译内核
all: $(TARGET)

# 链接内核
$(TARGET): $(OBJS) $(ASM_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "LD    $@"

# 编译 C 文件
# %.o 依赖 %.c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC    $<"

# 编译汇编文件
# %.o 依赖 %.S
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "AS    $<"

# 包含依赖文件 (如果存在)
-include $(DEPS)

# 运行 QEMU
run: $(TARGET)
	qemu-system-riscv64 -machine virt -kernel $(TARGET) -nographic

# 调试模式运行 (QEMU 等待 GDB 连接)
debug: $(TARGET)
	qemu-system-riscv64 -machine virt -kernel $(TARGET) -nographic -s -S

# 反汇编 (生成 kernel.asm 用于检查编译结果)
disasm: $(TARGET)
	$(OBJDUMP) -S $(TARGET) > kernel/kernel.asm
	@echo "Disassembly generated in kernel/kernel.asm"

# 显示构建信息
info:
	@echo "Sources: $(SRCS)"
	@echo "ASM Sources: $(ASM_SRCS)"
	@echo "Objects: $(OBJS)"

# 清理构建文件
clean:
	rm -f kernel/*.o kernel/*.d $(TARGET) kernel/kernel.asm
	@echo "Clean complete"