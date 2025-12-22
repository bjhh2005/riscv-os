#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include <stdarg.h>

struct spinlock pr;

void
printfinit(void)
{
  initlock(&pr, "pr");
}

// [关键修复] 将 int 改为 long (64位)
static void
printint(long xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[32]; // 增加缓冲区大小以容纳64位数字
  int i;
  uint64 x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    console_putc(buf[i]);
}

void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

  int locking = pr.locked;
  if(!locking) acquire(&pr);

  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      console_putc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      // [关键修复] 显式转换 long
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      // [关键修复] 显式转换 uint64 (也就是 long)
      printint((uint64)va_arg(ap, void*), 16, 1);
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        console_putc(*s);
      break;
    case 'c':  // [新增] 处理 %c
      console_putc(va_arg(ap, int));
      break;
    case '%':
      console_putc('%');
      break;
    default:
      console_putc('%');
      console_putc(c);
      break;
    }
  }
  va_end(ap);

  if(!locking) release(&pr);
}

// [同样修复 sprintf]
static void
sprintint(char **buf, long xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char temp[32];
  int i;
  uint64 x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    temp[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    temp[i++] = '-';

  while(--i >= 0)
    *(*buf)++ = temp[i];
}

void
sprintf(char *dst, char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;
  char *d = dst;

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      *d++ = c;
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      sprintint(&d, va_arg(ap, int), 10, 1);
      break;
    case 'x':
      sprintint(&d, va_arg(ap, int), 16, 1);
      break;
    case 'p':
      sprintint(&d, (uint64)va_arg(ap, void*), 16, 1);
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      while(*s)
        *d++ = *s++;
      break;
    case 'c': // [新增] 处理 %c
      *d++ = va_arg(ap, int);
      break;
    case '%':
      *d++ = '%';
      break;
    default:
      *d++ = '%';
      *d++ = c;
      break;
    }
  }
  *d = '\0';
  va_end(ap);
}

void
panic(char *s)
{
  printf("panic: ");
  printf(s);
  printf("\n");
  for(;;)
    ;
}