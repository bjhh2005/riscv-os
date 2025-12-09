#ifndef _PRINTF_H_
#define _PRINTF_H_

#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// --- 函数声明 ---
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
void panic(const char *s);

#endif