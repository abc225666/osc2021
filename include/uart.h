#ifndef UART_H
#define UART_H

#include "queue.h"

struct queue *uart_buffer_read;
struct queue *uart_buffer_write;

void uart_init();
void uart_putchar(unsigned int c);
void uart_putstr(char* str);
void async_putstr(char* str);
char uart_getchar();
char uart_read_raw();
void uart_printf(char *fmt, ...);
void async_printf(char *fmt, ...);

char async_getchar();
void async_putchar(char c);
void uart_buffer_init();


#endif
