#include "types.h"
#include "riscv.h"
#include "printf.h"

// --- 外部声明 ---
extern void kernelvec();
extern void sbi_set_timer(uint64 stime_value);

// --- 宏定义 ---
#define MAX_IRQS 16
#define IRQ_TIMER 5  // Supervisor Timer Interrupt (scause exception code)

#define CLOCK_FREQ 10000000 
#define INTERVAL (CLOCK_FREQ / 100) // 10ms一次 (用于测试)

// --- 数据结构定义 ---

// 中断处理函数指针类型
typedef void (*irq_handler_t)(void *arg);

// 中断描述符
struct irq_action {
    irq_handler_t handler; // 处理函数
    void *arg;             // 传递给函数的参数
    int used;              // 是否已注册
    uint64 count;          // 触发次数统计 (功能/性能测试用)
    char *name;            // 中断名称 (调试用)
};

// 全局中断向量表
struct irq_action irq_table[MAX_IRQS];

// 全局 tick 计数
static uint64 ticks;

// --- 辅助函数：读取 CPU 周期 (用于性能测试) ---
static inline uint64 read_cycle() {
    uint64 x;
    asm volatile("rdcycle %0" : "=r" (x) );
    return x;
}

// --- 1. 中断框架实现 (注册/注销) ---

// 注册中断处理函数
void irq_register(int irq, irq_handler_t handler, void *arg, char *name) {
    if(irq < 0 || irq >= MAX_IRQS) panic("irq_register: invalid irq");
    if(irq_table[irq].used) panic("irq_register: irq already used");

    irq_table[irq].handler = handler;
    irq_table[irq].arg = arg;
    irq_table[irq].name = name;
    irq_table[irq].used = 1;
    irq_table[irq].count = 0;

    printf("IRQ Manager: Registered %s for IRQ %d\n", name, irq);
}

// --- 2. 具体的中断处理逻辑 ---

// 时钟中断处理函数 (现在符合 irq_handler_t 签名)
void timer_handler(void *arg) {
    // 设置下一次中断
    sbi_set_timer(r_time() + INTERVAL);
    
    ticks++;
    printf("timer: tick %d (total interrupts: %d)\n", ticks, ((struct irq_action*)arg)->count + 1);
    
    // 在这里未来可以添加 yield() 触发调度
}

// --- 3. 初始化部分 ---

// 初始化中断子系统
void trapinit() {
    // 1. 清空中断表
    for(int i=0; i<MAX_IRQS; i++) {
        irq_table[i].used = 0;
        irq_table[i].count = 0;
    }

    // 2. 注册时钟中断
    // 将 timer_handler 注册到 IRQ 5，参数指向自己的描述符以便统计
    irq_register(IRQ_TIMER, timer_handler, &irq_table[IRQ_TIMER], "System Timer");
    
    printf("trapinit: Interrupt vector table initialized.\n");
}

// 每个 CPU 核心的硬件初始化
void trapinithart() {
    // 1. 设置中断向量基址
    w_stvec((uint64)kernelvec);
    
    // 2. 开启时钟中断使能位 (SIE.STIE)
    w_sie(r_sie() | SIE_STIE);
    
    // 3. 首次设置时钟，启动心跳
    sbi_set_timer(r_time() + INTERVAL);
    
    printf("trapinithart: HART hardware configured.\n");
}

// --- 4. 中断分发器 ---

// 设备中断分发
// 返回 1 表示处理成功，0 表示未处理
int devintr() {
    uint64 scause = r_scause();

    // 检查是否是中断 (最高位为1)
    if (scause & SCAUSE_INTERRUPT) {
        int irq = scause & 0xff; // 获取低位的中断号
        
        // 查表分发
        if (irq < MAX_IRQS && irq_table[irq].used) {
            struct irq_action *act = &irq_table[irq];
            
            act->count++; // 增加统计计数
            act->handler(act->arg); // 调用注册的函数
            return 1;
        } else {
            printf("devintr: unexpected interrupt irq=%d\n", irq);
        }
    }
    return 0;
}

// --- 5. 内核中断入口 (C语言部分) ---

void kerneltrap() {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();

    // 性能测试：记录开始时间
    uint64 start_time = read_cycle();

    // 安全检查
    if((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");
    if(intr_get() != 0)
        panic("kerneltrap: interrupts enabled");

    // 调用分发器
    if(devintr() == 0) {
        // 如果不是设备中断，那就是异常
        printf("scause=0x%p sepc=0x%p stval=0x%p\n", scause, sepc, r_stval());
        panic("kerneltrap: unexpected trap");
    }

    // 性能测试：记录结束时间并计算开销
    uint64 end_time = read_cycle();
    uint64 duration = end_time - start_time;

    // 为了避免刷屏，只在特定的 tick 或者开销异常大时打印
    // 这里为了演示，每 10 次 tick 打印一次性能数据
    if (ticks % 10 == 0 && ticks > 0) {
        printf("[Perf] IRQ handling took %d cycles\n", duration);
    }

    // 恢复上下文
    w_sepc(sepc);
    w_sstatus(sstatus);
}