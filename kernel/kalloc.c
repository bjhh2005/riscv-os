#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "kalloc.h"
#include "printf.h"
#include "string.h"

extern char end[]; // 内核代码结束位置

#define MAX_ORDER 10  // 最大阶数 2^10 * 4KB = 4MB
#define RUN_BASE ((uint64)end)
#define RUN_TOP  PHYSTOP

// 计算总页数 (128MB / 4KB = 32768页)
// 这里的数组用于在页面分配出去后，记录它是几阶的，以便free时合并
// 这是一个简单的外部元数据管理方案
#define MAX_PAGES ((PHYSTOP - 0x80000000) / PGSIZE) 
uint8_t page_orders[MAX_PAGES]; 

struct run {
  struct run *next;
};

struct {
  struct run *freelists[MAX_ORDER + 1]; // 0~10阶的空闲链表
} kmem;

// 辅助函数：物理地址转页号（相对于0x80000000）
int pa2idx(uint64 pa) {
  return (pa - 0x80000000) / PGSIZE;
}

// 辅助函数：页号转物理地址
uint64 idx2pa(int idx) {
  return 0x80000000 + (uint64)idx * PGSIZE;
}

// 获取 buddy 的物理地址
uint64 get_buddy(uint64 pa, int order) {
  uint64 size = (1UL << order) * PGSIZE;
  return pa ^ size; // 异或操作找到伙伴
}

void kinit() {
  // 1. 初始化所有链表
  for(int i = 0; i <= MAX_ORDER; i++) {
    kmem.freelists[i] = 0;
  }

  // 2. 初始化 page_orders 数组
  memset(page_orders, 0, sizeof(page_orders));

  // 3. 将可用内存范围加入伙伴系统
  // 为了简化，我们按页逐个释放到系统中，让 buddy_free 自动合并它们
  char *p = (char*)PGROUNDUP((uint64)end);
  printf("kinit: initializing buddy system from %p to %p\n", p, (void*)PHYSTOP);
  
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE) {
    kfree(p); 
  }
}

// 核心分配函数
void *buddy_alloc(int order) {
  int cur_order;
  
  // 1. 寻找足够大的最小空闲块
  for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
    if (kmem.freelists[cur_order]) {
      // 2. 找到块，从链表移除
      struct run *r = kmem.freelists[cur_order];
      kmem.freelists[cur_order] = r->next;
      
      uint64 pa = (uint64)r;

      // 3. 如果块太大，进行分裂
      while (cur_order > order) {
        cur_order--;
        uint64 buddy_pa = get_buddy(pa, cur_order);
        struct run *buddy_r = (struct run *)buddy_pa;
        
        // 将分裂出来的伙伴块加入低一级的空闲链表
        buddy_r->next = kmem.freelists[cur_order];
        kmem.freelists[cur_order] = buddy_r;
        
        // 记录伙伴块的 order (虽然它在空闲链表中，但也标记一下以防万一)
        page_orders[pa2idx(buddy_pa)] = cur_order;
      }
      
      // 4. 记录分配出去的块的 order，供 kfree 使用
      page_orders[pa2idx(pa)] = order;
      
      // 5. 清零并返回 (必须返回对齐的地址！)
      memset((void*)pa, 0, (1 << order) * PGSIZE);
      return (void*)pa;
    }
  }
  
  return 0; // 内存不足
}

// 核心释放函数
void buddy_free(void *pa) {
  uint64 block_pa = (uint64)pa;
  
  // 简单的范围检查
  if(block_pa < (uint64)end || block_pa >= PHYSTOP) 
    return;

  int idx = pa2idx(block_pa);
  int order = page_orders[idx]; // 获取该块的大小

  // 尝试合并
  while (order < MAX_ORDER) {
    uint64 buddy_pa = get_buddy(block_pa, order);
    
    // --- 修复：删除了未使用的 buddy_idx 变量 ---
    
    // 检查伙伴是否越界
    if (buddy_pa < (uint64)end || buddy_pa >= PHYSTOP) break;

    // 检查伙伴是否空闲
    struct run **pprev = &kmem.freelists[order];
    struct run *curr = *pprev;
    int found_buddy = 0;

    while(curr) {
      if((uint64)curr == buddy_pa) {
        // 找到伙伴，将其从链表移除
        *pprev = curr->next;
        found_buddy = 1;
        break;
      }
      pprev = &curr->next;
      curr = *pprev;
    }

    if (!found_buddy) {
      break; // 伙伴已分配或拆分，无法合并
    }

    // 合并：取地址较小者作为新块
    if (buddy_pa < block_pa) {
      block_pa = buddy_pa;
    }
    order++;
  }

  // 将最终的块加入对应的空闲链表
  struct run *r = (struct run*)block_pa;
  r->next = kmem.freelists[order];
  kmem.freelists[order] = r;
  
  // 更新该块的 order
  page_orders[pa2idx(block_pa)] = order;
}

// 适配接口：分配一页
void *kalloc(void) {
  return buddy_alloc(0);
}

// 适配接口：释放内存
void kfree(void *pa) {
  if(pa == 0) return;
  buddy_free(pa);
}

// 适配接口：分配多页
// 计算需要的 order，向上取整
void *kalloc_pages(int n) {
    if (n <= 0) return 0;
    
    int order = 0;
    while((1 << order) < n) {
        order++;
    }
    
    if (order > MAX_ORDER) return 0;
    
    return buddy_alloc(order);
}