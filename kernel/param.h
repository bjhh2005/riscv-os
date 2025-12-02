#ifndef PARAM_INCLUDE
#define PARAM_INCLUDE

#define NPROC        64    // 最大进程数量
#define KSTACKSIZE   4096  // 每个进程的内核栈大小（字节）
#define NCPU         8     // 最大CPU核心数
#define NOFILE       16    // 每个进程可打开的最大文件数
#define NFILE        100   // 系统全局最大打开文件数
#define NBUF         10    // 磁盘块缓存大小
#define NINODE       50    // 最大活动i-node数量
#define NDEV         10    // 最大主设备号
#define ROOTDEV      1     // 根文件系统设备号
#define MAXARG       32    // 执行程序的最大参数数量
#define LOGSIZE      10    // 磁盘日志的最大数据扇区数
#define HZ           10    // 定时器频率（可选）
#define N_CALLSTK    15    // 调用栈深度（特定实现）

#endif

