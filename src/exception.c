#include "auxiliary.h"
#include "uart.h"
#include "timer.h"
#include "irq.h"
#include "queue.h"
#include "uart.h"
#include "thread.h"
#include "syscall.h"
#include "allocator.h"
#include "sysregs.h"
#include "string.h"
#include "exception_handler.h"

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
                case 3:
                    sys_exec(trapframe);
                    break;
                case 4:
                    sys_exit(trapframe);
                    break;
                case 5:
                    sys_schedule(trapframe);
                    break;
                case 6:
                    sys_fork(trapframe);
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

void sys_exit(struct trapframe *trapframe) {
    // release current thread
    struct thread_t *thread = get_current_thread();
    thread->state = ZOMBIE;
    kfree(thread->sp_base);
    asm volatile("msr elr_el1, %0" : : "r"(do_schedule));
    asm volatile("eret");
}

void sys_exec(struct trapframe *trapframe) {
    void *func = (void *)trapframe->x[0];
    char **argv = (char **)trapframe->x[1];
    
    int argc = 0;
    int i = 0;
    while(argv[i]!=0) {
        argc++;
        i++;
    }

    struct thread_t *thread = get_current_thread();
    thread->fptr = func;
    thread->context.lr = (unsigned long)exit_wrapper;
    thread->context.fp = (unsigned long)(thread->sp_base + KSTACK_SIZE);
    thread->context.sp = (unsigned long)(thread->sp_base + KSTACK_SIZE);
    thread->argc = argc;

    void *user_sp = thread->user_sp_base + KSTACK_SIZE;
    for(int i=argc-1;i>=0;i--) {
        user_sp -= sizeof(argv[i]);
        *(char **)user_sp = argv[i];
    }
    thread->user_sp = user_sp;

    trapframe->x[0] = 0;
    asm volatile("msr sp_el0, %0" : : "r"(thread->sp_base+KSTACK_SIZE));
    asm volatile("msr elr_el1, %0" : : "r"(exit_wrapper));
    asm volatile("msr spsr_el1, %0" : : "r"(SPSR_EL1_VALUE));
    asm volatile("eret");

}

void sys_schedule(struct trapframe *trapframe) {
    do_schedule();
}

void sys_fork(struct trapframe *trapframe) {
    // create new thread
    struct thread_t *new_thread = (struct thread_t *)kmalloc(sizeof(struct thread_t));

    // copy current thread info
    struct thread_t *cur_thread = get_current_thread();
    memcpy(new_thread, cur_thread, sizeof(struct thread_t));

    new_thread->tid = new_pid();
    uart_printf("fork pid: %d\n", new_thread->tid);
    // create self sp and copy
    new_thread->sp_base = kmalloc(KSTACK_SIZE);
    new_thread->user_sp_base = kmalloc(KSTACK_SIZE);
    uart_printf("fork create: %x %x\n", new_thread->sp_base, new_thread->user_sp_base);

    memcpy(new_thread->sp_base, cur_thread->sp_base, KSTACK_SIZE);
    memcpy(new_thread->user_sp_base, cur_thread->user_sp_base, KSTACK_SIZE);
    

    // set correct sp, fp in context
    new_thread->context.fp = (unsigned long)new_thread->sp_base + KSTACK_SIZE;
    new_thread->context.sp = trapframe->sp_el0 - (unsigned long)cur_thread->sp_base + (unsigned long)new_thread->sp_base;

    new_thread->user_sp = cur_thread->user_sp - cur_thread->user_sp_base + new_thread->user_sp_base;


    // append trapframe to fork process
    new_thread->context.sp -= sizeof(struct trapframe);
    memcpy(new_thread->context.sp, trapframe, sizeof(struct trapframe));


    // set correct lr
    //new_thread->context.lr = trapframe->x[30];
    new_thread->context.lr = exit_fork; 

    thread_push(new_thread);

    struct trapframe *child_trapframe = (struct trapframe *)new_thread->context.sp;
    child_trapframe->x[0] = 0;
    trapframe->x[0] = new_thread->tid;
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
