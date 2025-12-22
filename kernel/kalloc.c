#include "types.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern char end[]; // 内核代码结束位置

#define MAX_ORDER 10  // 最大阶数 2^10 * 4KB = 4MB

// 计算总页数 (128MB / 4KB = 32768页)
#define MAX_PAGES ((PHYSTOP - KERNBASE) / PGSIZE) 

// 记录每个物理页当前属于哪个 order (分配时记录，释放时读取)
uint8_t page_orders[MAX_PAGES]; 

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelists[MAX_ORDER + 1]; // 0~10阶的空闲链表
} kmem;

struct spinlock reflock;
int ref_counts[MAX_PAGES]; // 引用计数数组

// ----------------------------------------------------------------
// 辅助函数
// ----------------------------------------------------------------

// 物理地址转页号（相对于 0x80000000）
int pa2idx(uint64 pa) {
  return (pa - KERNBASE) / PGSIZE;
}

// 页号转物理地址
uint64 idx2pa(int idx) {
  return KERNBASE + (uint64)idx * PGSIZE;
}

// 获取 buddy 的物理地址
uint64 get_buddy(uint64 pa, int order) {
  uint64 size = (1UL << order) * PGSIZE;
  // 相对于基地址进行异或
  uint64 relative_pa = pa - KERNBASE;
  uint64 buddy_relative = relative_pa ^ size;
  return KERNBASE + buddy_relative;
}

// 前向声明
void buddy_free(void *pa);

// ----------------------------------------------------------------
// 初始化
// ----------------------------------------------------------------
void kinit() {
  initlock(&kmem.lock, "kmem");
  initlock(&reflock, "kref");

  // 1. 初始化所有链表
  for(int i = 0; i <= MAX_ORDER; i++) {
    kmem.freelists[i] = 0;
  }

  // 2. 初始化元数据
  memset(page_orders, 0, sizeof(page_orders));
  memset(ref_counts, 0, sizeof(ref_counts));

  // 3. 将可用内存范围加入伙伴系统
  // 注意：必须对齐到 PGROUNDUP
  char *p = (char*)PGROUNDUP((uint64)end);
  
  printf("kinit: buddy system managing [%p, %p]\n", p, (void*)PHYSTOP);
  
  acquire(&kmem.lock);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE) {
    // [关键修复] 初始化时直接调用 buddy_free
    // 不要调用 kfree，因为 kfree 会检查 ref_count，而此时 ref_count 还没建立
    buddy_free(p); 
  }
  release(&kmem.lock);
}

// ----------------------------------------------------------------
// 核心逻辑：分配 (Internal)
// 需要调用者持有 kmem.lock
// ----------------------------------------------------------------
void *buddy_alloc_locked(int order) {
  int cur_order;
  
  // 1. 寻找足够大的最小空闲块
  for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
    if (kmem.freelists[cur_order]) {
      // 2. 找到块，从链表移除
      struct run *r = kmem.freelists[cur_order];
      kmem.freelists[cur_order] = r->next;
      
      uint64 pa = (uint64)r;

      // 3. 如果块太大，进行分裂 (Split)
      while (cur_order > order) {
        cur_order--;
        uint64 buddy_pa = get_buddy(pa, cur_order);
        struct run *buddy_r = (struct run *)buddy_pa;
        
        // 将分裂出来的伙伴块加入低一级的空闲链表
        buddy_r->next = kmem.freelists[cur_order];
        kmem.freelists[cur_order] = buddy_r;
        
        // 记录伙伴块的 order
        page_orders[pa2idx(buddy_pa)] = cur_order;
      }
      
      // 4. 记录分配出去的块的 order
      page_orders[pa2idx(pa)] = order;
      
      // 5. 清零内存 (重要：防止旧数据泄漏)
      // 注意：这里用 0 填充，而不是 5，因为这块内存马上要用了
      memset((void*)pa, 0, (1 << order) * PGSIZE);
      return (void*)pa;
    }
  }
  
  return 0; // 内存不足
}

void *buddy_alloc(int order) {
  void *pa;
  acquire(&kmem.lock);
  pa = buddy_alloc_locked(order);
  release(&kmem.lock);
  return pa;
}

// ----------------------------------------------------------------
// 核心逻辑：释放 (Internal)
// 需要调用者持有 kmem.lock
// ----------------------------------------------------------------
void buddy_free(void *pa) {
  uint64 block_pa = (uint64)pa;
  
  // 简单的范围检查
  if(block_pa < KERNBASE || block_pa >= PHYSTOP) 
    panic("buddy_free: out of range");

  int idx = pa2idx(block_pa);
  int order = page_orders[idx]; // 获取该块的大小

  // 尝试合并 (Coalesce)
  while (order < MAX_ORDER) {
    uint64 buddy_pa = get_buddy(block_pa, order);
    
    // 检查伙伴是否越界
    if (buddy_pa < KERNBASE || buddy_pa >= PHYSTOP) break;

    // 检查伙伴是否在空闲链表中
    // 注意：这里需要遍历链表，效率较低，生产环境通常用 bitmap 或 page 标志位优化
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

// ----------------------------------------------------------------
// 外部接口 (kalloc, kfree, kref)
// ----------------------------------------------------------------

// [COW Step 2] 增加引用计数
void kref_inc(void *pa) {
  acquire(&reflock);
  int idx = pa2idx((uint64)pa);
  if (idx < 0 || idx >= MAX_PAGES) 
      panic("kref_inc: index out of range");
  ref_counts[idx]++;
  release(&reflock);
}

// 标准接口：分配一页 (Order 0)
void *kalloc(void) {
  void *pa = buddy_alloc(0);
  
  if(pa) {
      // 分配成功，引用计数置 1
      acquire(&reflock);
      int idx = pa2idx((uint64)pa);
      ref_counts[idx] = 1;
      release(&reflock);
  }
  return pa;
}

// 标准接口：分配多页 (供不需要引用计数的场景使用，或你需要手动管理引用)
void *kalloc_pages(int n) {
    if (n <= 0) return 0;
    
    int order = 0;
    while((1 << order) < n) {
        order++;
    }
    
    if (order > MAX_ORDER) return 0;
    
    void *pa = buddy_alloc(order);
    
    // 注意：kalloc_pages 也要设置引用计数，否则 kfree 会出错
    if (pa) {
        acquire(&reflock);
        // 这里简化处理：只设置首页的引用计数，或者你需要遍历设置？
        // 既然 kfree 是根据地址和 page_orders 来释放的，
        // 我们只需要保证传入 kfree 的那个地址有引用计数即可。
        int idx = pa2idx((uint64)pa);
        ref_counts[idx] = 1; 
        release(&reflock);
    }
    return pa;
}

// 标准接口：释放内存
void kfree(void *pa) {
  int idx = pa2idx((uint64)pa);
  
  if (idx < 0 || idx >= MAX_PAGES)
      panic("kfree: out of range");

  acquire(&reflock);
  if(ref_counts[idx] > 0) {
      ref_counts[idx]--;
  } else {
      // 如果本来就是0，说明出bug了
      panic("kfree: ref_counts underflow");
  }
  
  // 如果还有引用，则不真正释放
  if(ref_counts[idx] > 0) {
      release(&reflock);
      return;
  }
  release(&reflock);

  // 引用归零，真正的物理释放
  acquire(&kmem.lock);
  buddy_free(pa);
  release(&kmem.lock);
}