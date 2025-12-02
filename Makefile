# 工具链设置
CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

# 编译选项
CFLAGS = -mcmodel=medany -fno-pic -Wall -Werror -O2
CFLAGS += -Iinclude -nostdlib -ffreestanding -fno-builtin
CFLAGS += -MD -MP # 自动生成依赖关系，确保头文件修改后重新编译[9](@ref)

# 链接选项
LDFLAGS = -T kernel/kernel.ld -nostdlib -static

# 源文件列表（使用通配符自动查找，避免手动维护）[6](@ref)
SRCS = $(wildcard kernel/*.c)
OBJS = $(SRCS:.c=.o)
ASM_SRCS = $(wildcard kernel/*.S)
ASM_OBJS = $(ASM_SRCS:.S=.o)

# 最终目标文件
TARGET = kernel/kernel

# 依赖文件（用于自动重新编译）
DEPS = $(OBJS:.o=.d)

# 默认目标：编译内核
all: $(TARGET)

# 链接内核
$(TARGET): $(OBJS) $(ASM_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "LD    $@"

# 显示构建信息（调试用）
info:
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"
	@echo "ASM Objects: $(ASM_OBJS)"

# 编译C文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC    $<"

# 编译汇编文件
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "AS    $<"

# 包含自动生成的依赖关系[9](@ref)
-include $(DEPS)

# 运行QEMU
run: $(TARGET)
	qemu-system-riscv64 -machine virt -kernel $(TARGET) -nographic

# 调试模式运行
debug: $(TARGET)
	qemu-system-riscv64 -machine virt -kernel $(TARGET) -nographic -s -S

# 反汇编，用于调试
disasm: $(TARGET)
	$(OBJDUMP) -S $(TARGET) > kernel/kernel.asm

# 清理构建文件
clean:
	rm -f $(OBJS) $(ASM_OBJS) $(TARGET) $(DEPS) kernel/kernel.asm
	@echo "Clean complete"

# 伪目标声明[10](@ref)
.PHONY: all run debug disasm clean info