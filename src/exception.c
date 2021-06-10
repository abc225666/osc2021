#include "auxiliary.h"
#include "uart.h"
#include "timer.h"
#include "irq.h"
#include "queue.h"
#include "uart.h"
#include "thread.h"

void sync_exc_runner(unsigned long esr, unsigned long elr, unsigned long far) {
    unsigned long ec = (esr>>26) & 0b111111;
    unsigned long iss = esr & 0x1FFFFFF;
    if(ec==0b010101) { // from aarch64 svc
        switch(iss) {
            case 0:
                uart_printf("get exception 1\n");
                break;
            case 1:
                // sys_schedule
                schedule();
                break;
            default:
                uart_printf("undefined syscall %x\n", iss);
                break;
        } 
    }
    else {
        uart_printf("Exception class (EC): 0x%x\n", ec);
        uart_printf("esr: %x\n", esr);
        uart_printf("elr: %x\n", elr);
        uart_printf("ec: %x, iss: %x, far: %x\n", ec, iss, far);
    }
}

void uart_irq_handler() {
//    uart_disable_irq();
    if(*AUX_MU_IIR_REG & 0x4) { //read
        while(*AUX_MU_LSR_REG & 0x01) {
            char c = (char)(*AUX_MU_IO_REG);
            queue_push(uart_buffer_read, c);
        }
    }
    else if(*AUX_MU_IIR_REG & 0x2) { //write
        while(*AUX_MU_LSR_REG & 0x20) {
            if(is_queue_empty(uart_buffer_write)) {
                *AUX_MU_IER_REG &= ~(0x2); // disable write irq
                break;
            }

            char c = queue_pop(uart_buffer_write);
            *AUX_MU_IO_REG = c;
        }
    }
//    uart_enable_irq();
}

void irq_exc_runner() {
    unsigned int irq_pending = (unsigned int)*IRQ_PENDING1;

    // uart0 in irq 61 = 32+29
    if(irq_pending & (1<<(29))) {
        uart_irq_handler();
    }
    else {
        uart_printf("boot time: %d\n", get_boot_time());
        set_time_interval(2);
    }
}

void show_exception_status(int type, unsigned long esr, unsigned long elr) {
    uart_printf("error type %d\n", type);
    while(1);
}
