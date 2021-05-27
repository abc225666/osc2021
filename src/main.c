#include "allocator.h"
#include "uart.h"
#include "mystring.h"
#include "shell.h"
#include "mbox.h"
#include "timer.h"
#include "irq.h"
#include "thread.h"

#define CMDSIZE 128

extern void el0_start();

void foo(){
    for(int i = 0; i < 10; ++i) {
        struct thread_t *current_thread = get_current_thread();
        async_printf("Thread id: %d %d\n", current_thread->tid, i);
        schedule();
    }
}

void idle() {
    while(1) {
        schedule();
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

    thread_create(idle);
    thread_create(foo);
    thread_create(foo);


    // first thread
    struct thread_t init_thread;
    init_thread.tid = 0;
    init_thread.state = DEAD;
    init_thread.context.sp = 0x60000;
    update_current_thread(&init_thread);


    schedule();


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
