#include "uart.h"

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
