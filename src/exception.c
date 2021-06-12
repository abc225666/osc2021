#include "auxiliary.h"
#include "uart.h"
#include "timer.h"
#include "irq.h"
#include "queue.h"
#include "uart.h"
#include "thread.h"
#include "syscall.h"
#include "thread.h"

void irq_enable() {
    asm volatile("msr daifclr, #2");
}
void irq_disable() {
    asm volatile("msr daifset, #2");
}

void sync_exc_runner(unsigned long esr, unsigned long elr, struct trapframe *trapframe, unsigned long far) {
    unsigned long ec = (esr>>26) & 0b111111;
    unsigned long iss = esr & 0x1FFFFFF;
    if(ec==0b010101) { // from aarch64 svc
        unsigned long syscall_num = trapframe->x[8];
        if(iss != 0) {
            uart_printf("unknown entry %x\n", iss);
        }
        else {
            switch(syscall_num) {
                // getpid
                case 0:
                    sys_getpid(trapframe);
                    break;
                case 1:
                    sys_uart_read(trapframe);
                    break;
                case 2:
                    sys_uart_write(trapframe);
                    break;
                default:
                    uart_printf("undefined syscall %x\n", iss);
                    break;
            } 
        }
    }
    else {
        uart_printf("Exception class (EC): 0x%x\n", ec);
        uart_printf("esr: %x\n", esr);
        uart_printf("elr: %x\n", elr);
        uart_printf("ec: %x, iss: %x, far: %x\n", ec, iss, far);
    }
}

void sys_getpid(struct trapframe *trapframe) {
    unsigned long pid = get_current_thread()->tid;
    trapframe->x[0] = pid;
}

void sys_uart_read(struct trapframe *trapframe) {
    char *buf = (char *)trapframe->x[0];
    unsigned int size = trapframe->x[1];

    irq_enable();
    for(unsigned int i=0;i<size;i++) {
        buf[i] = async_getchar();
    }
    buf[size] = '\0';
    irq_disable();
    trapframe->x[0] = size;
}

void sys_uart_write(struct trapframe *trapframe) {
    const char *buf = (char *)trapframe->x[0];
    unsigned int size= trapframe->x[1];

    irq_enable();
    for(unsigned int i=0;i<size;i++) {
        async_putchar(buf[i]);
    }
    irq_disable();
    trapframe->x[0] = size;
}

void uart_irq_handler() {
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
