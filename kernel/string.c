#include "types.h"
#include "string.h"

void* memset(void *dst, int c, size_t n) {
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

int memcmp(const void *v1, const void *v2, size_t n) {
  const char *s1, *s2;
  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }
  return 0;
}

// memmove 可以处理内存重叠的情况，比 memcpy 安全
void* memmove(void *dst, const void *src, size_t n) {
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else {
    while(n-- > 0)
      *d++ = *s++;
  }
  return dst;
}

// memcpy 不处理重叠，但在非重叠情况下通常更快（这里简化实现）
void* memcpy(void *dst, const void *src, size_t n) {
  return memmove(dst, src, n);
}

size_t strlen(const char *s) {
  int n;
  for(n = 0; s[n]; n++)
    ;
  return n;
}

char* strcpy(char *dst, const char *src) {
  char *os = dst;
  while((*dst++ = *src++) != 0)
    ;
  return os;
}