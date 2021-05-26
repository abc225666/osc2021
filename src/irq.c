#include "auxiliary.h"
#include "irq.h"

void irq_init() {
    *AUX_MU_IER_REG |= 0x1;
    *IRQ_ENABLE_0 |= (1<<29);
}

void uart_enable_irq() {
    *IRQ_ENABLE_0 |= (1<<29);
}

void uart_disable_irq() {
    *IRQ_ENABLE_0 &= ~(1<<29);
}
