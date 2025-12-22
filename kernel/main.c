#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

// 标记系统是否已启动（用于多核同步，目前单核暂未用到，但保留好习惯）
volatile static int started = 0;

void
main()
{
  if(cpuid() == 0){
    // 1. 初始化控制台 (串口)
    console_init();
    printfinit();
    
    printf("\n");
    printf("xv6-riscv kernel is booting\n");
    printf("\n");

    // 2. 初始化内存子系统
    kinit();         // 物理内存分配器 (Buddy System + COW Step 2)
    kvminit();       // 创建内核页表
    kvminithart();   // 开启分页机制

    // 3. 初始化进程与中断
    procinit();      // 进程表初始化 (Step 3 基础)
    trapinit();      // Trap 初始化
    trapinithart();  // 开启中断 (Step 1 时钟中断基础)

    // 4. [关键] 创建第一个用户进程 (Step 4 验证)
    // 这里会加载 initcode 机器码，准备进入用户态
    userinit();      

    __sync_synchronize();
    started = 1;
  } else {
    // 多核启动逻辑 (暂时保留)
    while(started == 0);
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();
    trapinithart();
  }

  // 5. 启动调度器
  // CPU 将开始运行 userinit 创建的进程
  printf("main: starting scheduler\n");
  scheduler();
}