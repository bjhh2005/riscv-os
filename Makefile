# 定义内核目录变量
K = kernel

# 工具链设置
CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump

# 编译选项
# -I. : 允许代码中使用 #include "kernel/defs.h" 这种路径
CFLAGS = -Wall -Werror -O2 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I. -fno-stack-protector
CFLAGS += -fno-pie -no-pie

# 链接选项
LDFLAGS = -z max-page-size=4096 -T $K/kernel.ld

# =================================================================
# 核心对象文件列表
# 注意：这里显式列出文件，是为了排除尚未实现的文件系统模块 (fs.c, exec.c 等)
# =================================================================
OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/kalloc.o \
  $K/spinlock.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/plic.o \
  $K/sbi.o \
  $K/kernelvec.o

# 下面这些是文件系统相关的，暂时注释掉
# 等实现 Step 5 时解开注释
# $K/bio.o \
# $K/fs.o \
# $K/log.o \
# $K/file.o \
# $K/pipe.o \
# $K/exec.o \

# 最终目标文件
TARGET = $K/kernel

# 伪目标
.PHONY: all clean qemu run

# 默认目标
all: $(TARGET)

# 链接内核
$(TARGET): $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJS)
	$(OBJDUMP) -S $(TARGET) > $K/kernel.asm
	$(OBJDUMP) -t $(TARGET) | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

# 编译 C 文件
$K/%.o: $K/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译汇编文件
$K/%.o: $K/%.S
	$(CC) $(CFLAGS) -c -o $@ $<

# 运行 QEMU (默认运行模式)
qemu: $(TARGET)
	qemu-system-riscv64 -machine virt -bios none -kernel $(TARGET) -m 128M -smp 1 -nographic -global virtio-mmio.force-legacy=false

# 调试模式 (等待 GDB 连接)
qemu-gdb: $(TARGET)
	qemu-system-riscv64 -machine virt -bios none -kernel $(TARGET) -m 128M -smp 1 -nographic -global virtio-mmio.force-legacy=false -S -gdb tcp::26000

run: qemu
# 清理构建文件
clean: 
	rm -f $K/*.o $K/*.d $K/*.asm $K/*.sym $(TARGET)