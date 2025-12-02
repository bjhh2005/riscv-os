#ifndef VM_H
#define VM_H

#include"types.h"
#include"memlayout.h"
pagetable_t uvmcreate(void);
uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int perm);
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz); // 解决隐式声明
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free); // 解决隐式声明
int mappages(pagetable_t pagetable, uint64_t va, uint64_t size, uint64_t pa, int perm);
pte_t *walk(pagetable_t pagetable, uint64_t va, int alloc);
void kvminit(void);
void kvminithart(void);
#endif