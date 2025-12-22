#include "types.h"
#include "riscv.h"
#include "printf.h"
#include "uart.h"
#include "spinlock.h"
#include "proc.h"
#include "kalloc.h"
#include "string.h"

// 外部初始化函数
extern void kinit(void);
extern void kvminit(void);
extern void kvminithart(void);
extern void trapinit(void);
extern void trapinithart(void);

// 全局测试锁 (用于同步 printf)
struct spinlock print_lock;

// 辅助：忙等待
void delay(int count) {
    volatile int i;
    for (i = 0; i < count; i++);
}

// ==========================================
//   测试场景 1: 计算密集型 (Math Worker)
// ==========================================
void worker_math(void) {
    // 【关键】调度器切换过来时持有 p->lock，必须先释放！
    release(&myproc()->lock);

    int pid = myproc()->pid;
    
    acquire(&print_lock);
    printf(GREEN "[Math] PID %d Started (Calcuating Fib)...\n" RESET, pid);
    release(&print_lock);

    // 计算斐波那契
    int a = 0, b = 1, temp;
    for(int i=0; i<5000000; i++) {
        temp = a + b; a = b; b = temp;
        // 模拟偶尔让出 CPU
        if(i % 1000000 == 0) yield();
    }

    acquire(&print_lock);
    printf(GREEN "[Math] PID %d Finished.\n" RESET, pid);
    release(&print_lock);

    // 退出变为 ZOMBIE
    acquire(&myproc()->lock);
    myproc()->state = ZOMBIE;
    sched();
}

// ==========================================
//   测试场景 2: 高频切换 (Ping Pong)
// ==========================================
void worker_yield(void) {
    release(&myproc()->lock);
    int pid = myproc()->pid;

    for(int i=0; i<5; i++) {
        acquire(&print_lock);
        printf(CYAN "[Yield] PID %d Tick %d (Giving up CPU)\n" RESET, pid, i);
        release(&print_lock);
        
        // 模拟 IO 等待，主动放弃 CPU
        yield(); 
    }

    acquire(&myproc()->lock);
    myproc()->state = ZOMBIE;
    sched();
}

// ==========================================
//   测试场景 3: 生产者-消费者 (Sync Test)
// ==========================================
// 共享缓冲区
struct {
    struct spinlock lock;
    int data;
    int full;
} buffer;

void producer(void) {
    release(&myproc()->lock);
    printf(YELLOW "[Prod] Producer Started.\n" RESET);

    for(int i=0; i<3; i++) {
        acquire(&buffer.lock);
        while(buffer.full) {
            // 缓冲区满，睡眠等待
            sleep(&buffer, &buffer.lock);
        }
        buffer.data = i * 10;
        buffer.full = 1;
        printf(YELLOW "[Prod] Produced: %d\n" RESET, buffer.data);
        
        wakeup(&buffer); // 唤醒消费者
        release(&buffer.lock);
        delay(500000); 
    }
    
    acquire(&myproc()->lock);
    myproc()->state = ZOMBIE;
    sched();
}

void consumer(void) {
    release(&myproc()->lock);
    printf(MAGENTA "[Cons] Consumer Started.\n" RESET);

    for(int i=0; i<3; i++) {
        acquire(&buffer.lock);
        while(!buffer.full) {
            // 缓冲区空，睡眠等待
            sleep(&buffer, &buffer.lock);
        }
        int item = buffer.data;
        buffer.full = 0;
        printf(MAGENTA "[Cons] Consumed: %d\n" RESET, item);
        
        wakeup(&buffer); // 唤醒生产者
        release(&buffer.lock);
    }
    
    acquire(&myproc()->lock);
    myproc()->state = ZOMBIE;
    sched();
}

// ==========================================
//   内核线程创建助手
// ==========================================
void create_kthread(void (*func)(), char *name) {
    struct proc *p = allocproc();
    if(!p) {
        printf("FAILED to create kthread: %s\n", name);
        return;
    }
    
    // 【Hack】手动设置上下文
    // ra 设置为函数入口
    p->context.ra = (uint64)func;
    // sp 设置为内核栈顶
    p->context.sp = p->kstack + PGSIZE;
    
    // 拷贝名字
    int len = strlen(name);
    if(len > 15) len = 15;
    memmove(p->name, name, len);
    p->name[len] = 0;

    // 设为就绪
    // acquire(&p->lock); // 防止死锁
    p->state = RUNNABLE;
    release(&p->lock);
}

// ==========================================
//   Main
// ==========================================
void main(void) {
    uart_init();
    printf("\n" RED "=== RISC-V OS: Process Management Experiment ===" RESET "\n");

    // 1. 系统初始化
    kinit();         
    kvminit();       
    kvminithart();   
    procinit();      // 初始化进程表
    trapinit();      
    trapinithart(); 
    intr_on();       
    
    initlock(&print_lock, "print_lock");
    initlock(&buffer.lock, "buffer_lock");

    printf("[INIT] Kernel initialized. Creating tests...\n");

    // 2. 创建测试进程
    
    // Test A: 两个高频切换进程
    create_kthread(worker_yield, "Yield-A");
    create_kthread(worker_yield, "Yield-B");

    // Test B: 一个计算进程 (它应该会在 Yield A/B 间隙中运行)
    create_kthread(worker_math, "Math-Worker");

    // Test C: 生产者消费者同步测试
    create_kthread(producer, "Producer");
    create_kthread(consumer, "Consumer");

    // 3. 启动调度器
    // 这行代码执行后，CPU 将被调度器接管，main 函数不会返回
    printf(RED "[SCHED] Starting Scheduler on CPU 0...\n" RESET);
    scheduler(); 

    panic("Scheduler returned!");
}