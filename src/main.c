#include "allocator.h"
#include "uart.h"
#include "mystring.h"
#include "shell.h"
#include "mbox.h"
#include "timer.h"
#include "irq.h"
#include "thread.h"
#include "syscall.h"

#define CMDSIZE 128

extern void el0_start();

void foo2(){
    for(int i = 0; i < 10; ++i) {
        async_printf("Thread id: %d %d\n", getpid(), i);
        do_schedule();
    }
}

void wow(int argc, char *argv[]) {
    async_printf("QQ\n", argv[0], argv[1]);
    async_putstr(argv[0]);
    async_putstr(argv[1]);
}

void foo(){
    for(int i = 0; i < 10; ++i) {
        async_printf("Thread id: %d %d\n", getpid(), i);
        if(i==2) {
            exit();
        }
        do_schedule();
    }
}

void demo_fork() {
    int wowo = 1;
    int pid = fork();
    async_printf("fork: %d, wowo: %d\n", pid, wowo);
}



void bar(int argc, char *argv[]) {
    async_putstr(argv[0]);
}

void idle() {
    async_printf("idle\n");
    while(1) {
        do_schedule();
    }
}

void kernel_main() {
    char cmd[CMDSIZE] = { 0 };
    int cmd_idx=0;
    mbox_arm_memory();
    uart_init();
    mm_init();
    uart_buffer_init();
    irq_init();
    thread_pool_init();

    thread_create(idle, 0, NULL);
    //thread_create(foo, 0, NULL);
    //thread_create(foo2, 0, NULL);
    thread_create(demo_fork, 0, NULL);

    // first thread
    struct thread_t init_thread;
    init_thread.tid = 0;
    init_thread.state = DEAD;
    init_thread.context.sp = 0x40000;
    update_current_thread(&init_thread);

    do_schedule();

    async_putstr("\r\n");
    async_putstr("# ");
    while(1) {
        char c = async_getchar();
        switch(c) {
            case '\n':
                cmd[cmd_idx] = 0;
                async_putstr("\n");
                shell_cmd(cmd);
                async_putstr("# ");
                memset(cmd, 0, CMDSIZE);
                cmd_idx = 0;
                break;
            case 127:
                if(cmd_idx) {
                    async_putstr("\b \b");
                    cmd_idx--;
                }
                break;
            default:
                if(c>31 && c<127) {
                    cmd[cmd_idx] = c;
                    cmd_idx++;
                    async_putchar(c);
                }
                break;
        }
    }
}
