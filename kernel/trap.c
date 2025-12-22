#include "types.h"
#include "riscv.h"
#include "printf.h"
#include "uart.h" 
#include "memlayout.h"

#define CLOCK_FREQ 10000000 
#define INTERVAL   (CLOCK_FREQ / 100) 

// --- 异常定义 ---
#define CAUSE_MISALIGNED_FETCH    0x0
#define CAUSE_FETCH_ACCESS        0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT          0x3
#define CAUSE_MISALIGNED_LOAD     0x4
#define CAUSE_LOAD_ACCESS         0x5
#define CAUSE_MISALIGNED_STORE    0x6
#define CAUSE_STORE_ACCESS        0x7
#define CAUSE_USER_ECALL          0x8
#define CAUSE_SUPERVISOR_ECALL    0x9
#define CAUSE_FETCH_PAGE_FAULT    0xc
#define CAUSE_LOAD_PAGE_FAULT     0xd
#define CAUSE_STORE_PAGE_FAULT    0xf

// --- 中断定义 ---
#define IRQ_TIMER 5
#define IRQ_SOFT  1
#define IRQ_UART0 10

// --- 结构体定义 ---
struct context {
    uint64 ra; uint64 sp; uint64 gp; uint64 tp;
    uint64 t0; uint64 t1; uint64 t2;
    uint64 s0; uint64 s1;
    uint64 a0; uint64 a1; uint64 a2; uint64 a3; uint64 a4; uint64 a5; uint64 a6; uint64 a7;
    uint64 s2; uint64 s3; uint64 s4; uint64 s5; uint64 s6; uint64 s7; uint64 s8; uint64 s9; uint64 s10; uint64 s11;
    uint64 t3; uint64 t4; uint64 t5; uint64 t6;
};

#define MAX_IRQS 16
typedef void (*irq_handler_t)(void *arg);
struct irq_action {
    irq_handler_t handler;
    void *arg;
    int used;
    char *name;
};
struct irq_action irq_table[MAX_IRQS];

static uint64 ticks = 0;
extern void sbi_set_timer(uint64 stime_value);

// --- 处理函数 ---

void timer_handler(void *arg) {
    sbi_set_timer(r_time() + INTERVAL);
    ticks++;
    // 关闭心跳打印，避免干扰控制台
}

void software_handler(void *arg) {
    printf(CYAN "\n[IRQ-Soft]" RESET " Software Interrupt Triggered! Clearing SIP...\n");
    uint64 mask = 2;
    asm volatile("csrc sip, %0" : : "r"(mask));
}

void uart_handler(void *arg) {
    volatile uint8 *uart_base = (uint8 *)0x10000000L;
    volatile uint8 *rhr = uart_base + 0; 
    volatile uint8 *lsr = uart_base + 5; 
    while ((*lsr & 1)) {
        char c = *rhr;
        printf("[IRQ-UART] Keyboard Input: '%c' (ASCII %d)\n", c, c);
    }
}

void handle_syscall(struct context *ctx) {
    uint64 syscall_num = ctx->a7;
    if (syscall_num == 1) { 
        printf("   -> Syscall Print: %s\n", (char*)ctx->a0);
        ctx->a0 = 0; 
    } else {
        ctx->a0 = -1; 
    }
}


void handle_fault(uint64 scause, uint64 sepc, uint64 stval, uint64 *new_epc) {
    char *fault_name = "Unknown";
    int is_fatal = 0;

    switch (scause) {
        case 2:  fault_name = "Illegal Instruction"; break;
        case 12: fault_name = "Instruction Page Fault"; is_fatal = 1; break;
        case 13: fault_name = "Load Page Fault"; is_fatal = 1; break;
        case 15: fault_name = "Store Page Fault"; is_fatal = 1; break;
        default: fault_name = "Unknown Trap"; break;
    }

    printf(RED "\n[System Trap] Exception Caught!" RESET "\n");
    printf("   Type: %s (scause=%d)\n", fault_name, (int)scause); 
    printf("   Addr: 0x%x (stval)\n", (unsigned int)stval); // 或者是 (uint64)
    printf("   Code: 0x%x (sepc)\n", (unsigned int)sepc);

    // 【关键修复】检查是否在内核区域发生错误
    // 0x80000000 以上通常是内核物理内存或映射区域
    // 如果在内核态发生缺页或非法指令，必须停止系统，不能跳过！
    if (sepc >= KERNBASE || is_fatal) {
        printf(RED "!!! KERNEL PANIC: Unrecoverable Fault in Kernel Mode !!!\n" RESET);
        printf(RED "System Halted. Please check the code at sepc.\n" RESET);
        while(1); // 死循环保留现场
    }

    // 只有在未来实现用户态(User Mode) Copy-On-Write 时，
    // 才在这里处理缺页中断。现在，所有异常一律视为致命。
    while(1);
}

// --- 注册与分发 ---

void irq_register(int irq, irq_handler_t handler, char *name) {
    if (irq < MAX_IRQS) {
        irq_table[irq].handler = handler;
        irq_table[irq].name = name;
        irq_table[irq].used = 1;
    }
}

void kerneltrap(struct context *ctx) {
    uint64 scause = r_scause();
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 stval = r_stval();

    // 判断中断还是异常
    if (scause & 0x8000000000000000L) {
        int irq = scause & 0xff;
        if (irq < MAX_IRQS && irq_table[irq].used) {
            irq_table[irq].handler(NULL);
        } else {
            printf("Unexpected interrupt irq=%d\n", irq);
        }
    } else {
        uint64 next_pc = sepc; 
        if (scause == CAUSE_SUPERVISOR_ECALL || scause == CAUSE_USER_ECALL) {
            handle_syscall(ctx);
            next_pc += 4; 
        } else {
            handle_fault(scause, sepc, stval, &next_pc);
        }
        sepc = next_pc;
    }

    w_sepc(sepc);
    w_sstatus(sstatus);
}

void trapinit() {
    irq_register(IRQ_TIMER, timer_handler, "Timer");
    irq_register(IRQ_SOFT, software_handler, "Software");
    irq_register(IRQ_UART0, uart_handler, "UART0");
    printf("trapinit: Handlers registered.\n");
}

void trapinithart() {
    extern void kernelvec();
    w_stvec((uint64)kernelvec);
    w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
    *(volatile uint8 *)(0x10000000L + 1) = 0x01; // UART Interrupt Enable
    sbi_set_timer(r_time() + INTERVAL);
}