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

// --- 5. SATP 构造宏 ---
#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

// --- 6. 类型定义 ---
typedef uint64 pte_t;
typedef uint64 *pagetable_t;


// ============================================
// CSR 寄存器定义 (Machine Mode & Supervisor Mode)
// ============================================

// --- mstatus (Machine Status) ---
#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.

static inline uint64 r_mstatus() {
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void w_mstatus(uint64 x) {
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// --- mepc (Machine Exception Program Counter) ---
static inline void w_mepc(uint64 x) {
  asm volatile("csrw mepc, %0" : : "r" (x));
}

// --- sstatus (Supervisor Status) ---
#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

static inline uint64 r_sstatus() {
  uint64 x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void w_sstatus(uint64 x) {
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

// --- M-Mode Interrupt Delegation ---
static inline uint64 r_medeleg() {
  uint64 x;
  asm volatile("csrr %0, medeleg" : "=r" (x) );
  return x;
}

static inline void w_medeleg(uint64 x) {
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

static inline uint64 r_mideleg() {
  uint64 x;
  asm volatile("csrr %0, mideleg" : "=r" (x) );
  return x;
}

static inline void w_mideleg(uint64 x) {
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

// --- Interrupt Enable (sie) ---
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software

static inline uint64 r_sie() {
  uint64 x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void w_sie(uint64 x) {
  asm volatile("csrw sie, %0" : : "r" (x));
}

// --- Interrupt Pending (sip) ---
#define SIP_SSIP (1L << 1)

static inline void w_sip(uint64 x) {
  asm volatile("csrw sip, %0" : : "r" (x));
}

// --- PMP (Physical Memory Protection) ---
static inline void w_pmpcfg0(uint64 x) {
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void w_pmpaddr0(uint64 x) {
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}

// --- Other Supervisor CSRs ---
static inline uint64 r_sepc() {
  uint64 x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

static inline void w_sepc(uint64 x) {
  asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64 r_scause() {
  uint64 x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

#define SCAUSE_INTERRUPT (1L << 63) // Interrupt flag

static inline uint64 r_stval() {
  uint64 x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

static inline uint64 r_stvec() {
  uint64 x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

static inline void w_stvec(uint64 x) {
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64 r_satp() {
  uint64 x;
  asm volatile("csrr %0, satp" : "=r" (x) );
  return x;
}

static inline void w_satp(uint64 x) {
  asm volatile("csrw satp, %0" : : "r" (x));
}

static inline void sfence_vma() {
  asm volatile("sfence.vma zero, zero");
}

static inline uint64 r_time() {
  uint64 x;
  asm volatile("csrr %0, time" : "=r" (x) );
  return x;
}

static inline uint64 r_tp() {
  uint64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline uint64 r_mhartid() {
  uint64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// --- Helper Functions ---

// 开启中断
static inline void intr_on() {
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// 关闭中断
static inline void intr_off() {
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// 获取当前中断状态
static inline int intr_get() {
  uint64 x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

// 读取周期计数器 (用于性能测试)
static inline uint64 r_cycle() {
  uint64 x;
  asm volatile("rdcycle %0" : "=r" (x) );
  return x;
}

#endif