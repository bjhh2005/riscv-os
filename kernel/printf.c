#include "printf.h"
#include "console.h"
#include "string.h"
#include <stdarg.h>
#include <stdint.h>

// 数字字符查找表
static const char digits[] = "0123456789abcdef";

// 打印数字到控制台（带符号处理）
static void print_num(int64_t num, int base, int sign, int width, int zero_pad) {
    char buf[20]; // 足够存放64位二进制数
    int i = 0;

    // 处理负数
    if (sign && num < 0) {
        console_putc('-');
        num = -num;
    }

    // 特殊处理INT64_MIN
    if (num == INT64_MIN) {
        buf[i++] = '8'; // -2^63的最后一位
        num = INT64_MAX / base;
    }

    // 数字转换
    do {
        buf[i++] = digits[num % base];
    } while ((num /= base) > 0);

     // 零填充或空格填充
    while (i < width) {
        console_putc(zero_pad ? '0' : ' ');
        width--;
    }

    // 逆序输出
    while (--i >= 0) {
        console_putc(buf[i]);
    }
}

// 将数字写入缓冲区（返回写入字符数）
static int write_num(char *buf, int64_t num, int base, int sign) {
    char tmp[20];
    int i = 0, start = 0;

    if (sign && num < 0) {
        *buf++ = '-';
        start++;
        num = -num;
    }

    do {
        tmp[i++] = digits[num % base];
    } while ((num /= base) > 0);

    while (--i >= 0) {
        *buf++ = tmp[i];
        start++;
    }
    return start;
}

// 安全字符串打印
static void print_str(const char *s) {
    if (!s) s = "(null)";
    while (*s) console_putc(*s++);
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            console_putc(*fmt++);
            continue;
        }

        fmt++; // 跳过%

        int width = 0;
        int zero_pad = 0;

        // 处理填充和宽度
        if (*fmt == '0') {
            zero_pad = 1;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt++ - '0');
        }

        switch (*fmt++) {
            case 'd': 
                print_num(va_arg(ap, int), 10, 1, width, zero_pad); 
                break;
            case 'u':
                print_num(va_arg(ap, unsigned), 10, 0, width, zero_pad);
                break;
            case 'x': print_num(va_arg(ap, unsigned), 16, 0, width, zero_pad); break;
            case 's': print_str(va_arg(ap, char*)); break;
            case 'c': console_putc(va_arg(ap, int)); break;
            case '%': console_putc('%'); break;
            default:  // 回显无效格式
                console_putc('%');
                console_putc(fmt[-1]);
        }
    }

    va_end(ap);
    return 0;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    char *start = buf;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            *buf++ = *fmt++;
            continue;
        }

        fmt++; // 跳过%
        switch (*fmt++) {
            case 'd': buf += write_num(buf, va_arg(ap, int), 10, 1); break;
            case 'u': buf += write_num(buf, va_arg(ap, unsigned), 10, 0); break;
            case 'x': buf += write_num(buf, va_arg(ap, unsigned), 16, 0); break;
            case 's': {
                char *s = va_arg(ap, char*);
                if (!s) s = "(null)";
                buf += strlen(strcpy(buf, s));
                break;
            }
            case 'c': *buf++ = va_arg(ap, int); break;
            case '%': *buf++ = '%'; break;
            default:  // 回显无效格式
                *buf++ = '%';
                *buf++ = fmt[-1];
        }
    }

    *buf = '\0';
    va_end(ap);
    return buf - start;
}

void panic(const char *s) {
    printf("PANIC: %s\n", s ? s : "Unknown error");
    for(;;); // 系统挂起
}