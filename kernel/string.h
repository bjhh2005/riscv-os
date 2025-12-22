#ifndef STRING_H
#define STRING_H

#include "types.h"

// 基础内存/字符串操作
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
void *memset(void *dst, int c, size_t n);
void *memmove(void *dst, const void *src, size_t n); // 新增
void *memcpy(void *dst, const void *src, size_t n);  // 新增
int memcmp(const void *v1, const void *v2, size_t n);// 新增

#endif