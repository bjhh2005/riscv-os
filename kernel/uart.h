#ifndef UART_H
#define UART_H

void uart_putc(char c);
void uart_puts(char *s);
void uart_init();

int puts(const char *s);
int putchar(int c);

#endif