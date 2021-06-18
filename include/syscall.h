#ifndef SYSCALL_H
#define SYSCALL_H

struct trapframe {
    unsigned long x[31];
    unsigned long sp_el0;
    unsigned long elr_el1;
    unsigned long spsr_el1;
};

int getpid();
unsigned int kuart_read(char buf[], unsigned int size);
unsigned int kuart_write(char buf[], unsigned int size);
int exec(const char *name, char *const argv[]);
void schedule();
void exit();
int fork();

void exit_fork();

void sys_getpid(struct trapframe *trapframe);
void sys_uart_read(struct trapframe *trapframe);
void sys_uart_write(struct trapframe *trapframe);
void sys_exec(struct trapframe *trapframe);
void sys_schedule(struct trapframe *trapframe);
void sys_exit(struct trapframe *trapframe);
void sys_fork(struct trapframe *trapframe);

#endif
