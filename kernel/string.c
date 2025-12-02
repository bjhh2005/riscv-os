#include "string.h"

// 计算字符串长度（不包括结尾的 '\0'）
size_t strlen(const char *s) {
    const char *p = s;
    while (*p != '\0') {
        p++;
    }
    return p - s;
}

// 复制字符串（包括结尾的 '\0'）
char *strcpy(char *dst, const char *src) {
    char *p = dst;
    while (*src != '\0') {
        *p++ = *src++;
    }
    *p = '\0';  // 确保以 '\0' 结尾
    return dst;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    unsigned char val = (unsigned char)c;
    while (n--) {
        *p++ = val;
    }
    return s;
}