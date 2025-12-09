// 虚拟内存管理相关实现
#include "types.h"
#include "kalloc.h"
#include "vm.h"
#include "printf.h"
#include "stddef.h"
#include "string.h"
#include "memlayout.h"
#include "riscv.h"

// 内核页表全局变量
pagetable_t kernel_pagetable = 0;

// 内存区域映射辅助函数
static void map_region(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm) {
    for (uint64_t a = 0; a < size; a += PGSIZE) {
        mappages(pt, va + a, PGSIZE, pa + a, perm);
    }
}

extern char etext[];

void kvminit(void) {
    // 1. 创建内核页表
    kernel_pagetable = uvmcreate();
    
    // 2. 映射内核代码段（R+X）
    map_region(kernel_pagetable, KERNBASE, KERNBASE, (uint64_t)etext - KERNBASE, PTE_R | PTE_X);
    
    // 3. 映射内核数据段（R+W）
    map_region(kernel_pagetable, (uint64_t)etext, (uint64_t)etext, PHYSTOP - (uint64_t)etext, PTE_R | PTE_W);
    
    // 4. 映射 UART 设备 (R+W)
    map_region(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);

    // --- 新增：映射 CLINT (用于 sleep / timer) ---
    // CLINT 通常占用 0x10000 (64KB)
    map_region(kernel_pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);

    // --- 新增：映射 PLIC (用于中断控制器) ---
    // PLIC 通常占用 0x400000 (4MB)
    map_region(kernel_pagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);
}

void kvminithart(void) {
    // 激活内核页表
    uint64_t satp = (8L << 60) | (((uint64_t)kernel_pagetable >> 12) & 0xFFFFFFFFFFF);
    w_satp(satp);
    sfence_vma();
}

// 递归销毁页表，释放所有页表页（不释放映射的物理页）
void destroy_pagetable(pagetable_t pt)
{
    for (int i = 0; i < 512; i++) {
        pte_t pte = pt[i];
        if ((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0) {
            // 有效且为中间页表
            pagetable_t child = (pagetable_t)PTE2PA(pte);
            destroy_pagetable(child);
        }
    }
    kfree(pt);
}

// 打印页表内容，递归打印每级页表
void dump_pagetable(pagetable_t pt, int level)
{
    for (int i = 0; i < 512; i++) {
        pte_t pte = pt[i];
        if (pte & PTE_V) {
            for (int l = 0; l < level; l++)
                printf("  ");
            printf("[%d] pte=0x%lx", i, pte);
            if (pte & (PTE_R|PTE_W|PTE_X)) {
                printf(" -> leaf, pa=0x%lx\n", PTE2PA(pte));
            } else {
                printf(" -> next level\n");
                dump_pagetable((pagetable_t)PTE2PA(pte), level+1);
            }
        }
    }
}


// 遍历多级页表，返回虚拟地址 va 在页表中的页表项指针
// alloc=1 时，若中间页表不存在则自动分配
// 返回值：指向页表项的指针，失败返回 NULL
pte_t *walk(pagetable_t pagetable, uint64_t va, int alloc)
{
    // Sv39 虚拟地址最大 39 位
    if (va >= (1L << 39))
        return NULL;
    // 从顶级（2）到次级（1）依次查找
    for (int level = 2; level > 0; level--) {
        // 取当前层的页表项指针
        pte_t *pte = &pagetable[PX(level, va)];
        if (*pte & PTE_V) {
            // 有效则跳转到下一层页表
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            // 无效且需要分配新页表
            if (!alloc || (pagetable = (pagetable_t)kalloc()) == NULL)
                return NULL;
            memset(pagetable, 0, PGSIZE); // 新页表清零
            *pte = PA2PTE((uint64_t)pagetable) | PTE_V; // 建立映射
        }
    }
    // 返回第0级页表项指针
    return &pagetable[PX(0, va)];
}


// 在页表 pagetable 中建立从 va 到 pa 的映射，大小 size，权限 perm
// va/pa/size 必须页对齐
// perm: 权限位（如PTE_R|PTE_W等）
// 返回 0 成功，-1 失败
int mappages(pagetable_t pagetable, uint64_t va, uint64_t size, uint64_t pa, int perm)
{
    uint64_t a, last;
    pte_t *pte;
    a = PGROUNDDOWN(va); // 起始虚拟地址页对齐
    last = PGROUNDDOWN(va + size - 1); // 结束虚拟地址页对齐
    for (;;) {
        pte = walk(pagetable, a, 1); // 查找/分配页表项
        if (pte == NULL)
            return -1;
        if (*pte & PTE_V)
            return -1; // 已经映射，报错
        *pte = PA2PTE(pa) | perm | PTE_V; // 建立映射
        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}


// 创建一个空的用户页表（顶级页表）
// 返回新分配的页表指针，失败返回 NULL
pagetable_t uvmcreate(void)
{
    pagetable_t pagetable;
    pagetable = (pagetable_t)kalloc(); // 分配一页作为顶级页表
    if (pagetable == NULL)
        return NULL;
    memset(pagetable, 0, PGSIZE); // 清零
    return pagetable;
}