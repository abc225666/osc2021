#include "uart.h"
#include "timer.h"

void sync_exc_runner(unsigned long esr, unsigned long elr) {
    unsigned long ec = (esr>>26) & 0b111111;
    unsigned long iss = esr & 0x1FFFFFF;
    if(ec==0b010101) { // from aarch64 svc
        switch(iss) {
            case 0:
                uart_printf("get exception 1\n");
                break;
            default:
                uart_printf("undefined syscall\n");
        } 
    }
    else {
        uart_printf("Exception class (EC): 0x%x\n", ec);
        uart_printf("esr: %x\n", esr);
    }
}

void irq_exc_runner() {
    uart_printf("boot time: %d\n", get_boot_time());
    set_time_interval(2);
}

void show_exception_status(int type, unsigned long esr, unsigned long elr) {
    uart_printf("error type %d\n", type);
    while(1);
}
