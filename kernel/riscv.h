#ifndef __RISCV_H__
#define __RISCV_H__

#include "types.h"

// --- 1. 基础硬件定义 ---
#define PGSIZE 4096
#define PGSHIFT 12

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// --- 2. 页表项 (PTE) 标志位 ---
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

// --- 3. 地址转换宏 ---
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)
#define PTE2PA(pte) (((pte) >> 10) << 12)
#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// --- 4. 页表索引宏 (Sv39) ---
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((uint64)(va)) >> PXSHIFT(level)) & 0x1FF)

// Sv39 最大虚拟地址
#define MAXVA (1L << 38)

// --- 5. 关键寄存器操作 (这是你之前缺少的！) ---

// 读取 satp
static inline uint64 r_satp() {
  uint64 x;
  asm volatile("csrr %0, satp" : "=r" (x) );
  return x;
}

// 写入 satp
static inline void w_satp(uint64 x) {
  asm volatile("csrw satp, %0" : : "r" (x));
}

// 刷新 TLB
static inline void sfence_vma() {
  asm volatile("sfence.vma zero, zero");
}

// 读取 tp (线程指针/Core ID)
static inline uint64 r_tp() {
  uint64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

// --- 6. SATP 构造宏 ---
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

// --- 7. 类型定义 ---
typedef uint64 pte_t;
typedef uint64 *pagetable_t;

#endif