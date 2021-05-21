#include "timer.h"

void core_timer_enable() {
    asm volatile("msr cntp_ctl_el0, %0": : "r"(1));
    unsigned long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0": "=r"(cntfrq_el0));
    asm volatile("msr cntp_tval_el0, %0": : "r"(cntfrq_el0*2));

    asm volatile("mov x0, 2");
    asm volatile("ldr x1, =0x40000040");
    asm volatile("str x0, [x1]");
}

void set_time_interval(unsigned long interval) {
    unsigned long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0": "=r"(cntfrq_el0));
    asm volatile("msr cntp_tval_el0, %0": : "r"(cntfrq_el0*interval));
}

unsigned long get_boot_time() {
    unsigned long cntpct_el0, cntfrq_el0;
    asm volatile("mrs %0, cntpct_el0":"=r"(cntpct_el0));
    asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq_el0));
    return cntpct_el0 / cntfrq_el0;
}
