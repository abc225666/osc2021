#include "allocator.h"
#include "uart.h"
#include "mystring.h"
#include "shell.h"
#include "mbox.h"
#include "timer.h"
#include "irq.h"

#define CMDSIZE 128

extern void el0_start();

void kernel_main() {
    char cmd[CMDSIZE] = { 0 };
    int cmd_idx=0;
    mbox_arm_memory();
    uart_init();
    mm_init();
    uart_buffer_init();
    irq_init();


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
