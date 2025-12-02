#ifndef KALLOC_H
#define KALLOC_H

// 物理内存分配器头文件
// 声明内核物理内存管理接口

/**
 * 初始化物理内存分配器
 * 将可用物理内存（从end到PHYSTOP）加入到空闲链表
 */
void kinit(void);

/**
 * 分配一个物理页（4KB）
 * @return 成功返回页起始地址，失败返回0
 */
void* kalloc(void);

/**
 * 释放一个物理页
 * @param pa 要释放的页的起始地址（必须页对齐）
 */
void kfree(void *pa);

/**
 * 分配连续的多个物理页
 * @param n 请求的页数
 * @return 成功返回第一页的起始地址，失败返回0
 * @note 此实现不保证物理连续性，实际系统中需要更复杂的算法
 */
void* kalloc_pages(int n);

#endif // KALLOC_H