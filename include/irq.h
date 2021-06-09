#ifndef IRQ_H
#define IRQ_H

#include "mmio.h"

#define IRQ_BASE (MMIO_BASE+0xb000)

#define IRQ_PENDING1 ((unsigned int*)(IRQ_BASE+0x204))
#define IRQ_ENABLE_0 ((unsigned int*)(IRQ_BASE+0x210))

extern void _el0_start(void* addr);

void irq_init();
void uart_enable_irq();
void uart_disable_irq();

#endif
