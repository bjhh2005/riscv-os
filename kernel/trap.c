#include "types.h"
#include "riscv.h"
#include "printf.h"
#include "uart.h" 

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

// 智能异常恢复逻辑 (修复版)
void handle_fault(uint64 scause, uint64 sepc, uint64 stval, uint64 *new_epc) {
    char *fault_name = "Unknown";
    int is_fatal = 0; // 标记是否致命

    switch (scause) {
        case CAUSE_ILLEGAL_INSTRUCTION: fault_name = "Illegal Instruction"; break;
        case CAUSE_STORE_PAGE_FAULT:    fault_name = "Store Page Fault"; break;
        case CAUSE_LOAD_PAGE_FAULT:     fault_name = "Load Page Fault"; break;
        
        // 【关键】必须拦截断点，否则跳过会导致跑飞
        case CAUSE_BREAKPOINT:          fault_name = "Breakpoint"; is_fatal = 1; break;
        // 【关键】如果取指本身出错，不能读取指令，必须停机
        case CAUSE_FETCH_PAGE_FAULT:    fault_name = "Instruction Page Fault"; is_fatal = 1; break;
    }

    printf(RED "\n[System Trap] Exception Caught!" RESET "\n");
    printf("   Type: %s (scause=%d)\n", fault_name, scause);
    printf("   Addr: 0x%p (stval)\n", stval);
    printf("   Code: 0x%p (sepc)\n", sepc);

    if (is_fatal) {
        printf(RED "   [FATAL] Cannot recover from this exception. System Halted.\n" RESET);
        while(1); // 死循环停机
    }

    // 读取指令判断长度 (普通4字节，压缩2字节)
    uint32 instr_opcode = *(uint16_t*)sepc; 
    int step = 2; 

    if ((instr_opcode & 0x3) == 0x3) {
        step = 4; 
    }

    printf(YELLOW "   -> Action: Skipping %d-byte instruction...\n" RESET, step);
    *new_epc = sepc + step;
    
    // 延时，防止连续错误刷屏
    for(volatile int i=0; i<2000000; i++);
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