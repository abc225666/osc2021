#include "allocator.h"
#include "cpio.h"
#include "mystring.h"
#include "uart.h"
#include "utils.h"
#include "sysregs.h"
#include "timer.h"
#include "cpio.h"

extern void el0_start();

void shell_cmd(char* cmd) {
    if(!strcmp(cmd, "help")) {
        uart_putstr("help: print all available commands\n");
        uart_putstr("hello: print Hello World!\n");
        uart_putstr("reboot: reboot rpi\n");
        uart_putstr("loadimg: start loadimg from uart\n");
        uart_putstr("cpio: read cpio data\n");
        uart_putstr("alloc: alloc mem\n");
        uart_putstr("free: free mem\n");
    }
    else if(!strcmp(cmd, "hello")) {
        uart_putstr("Hello World!\n");
    }
    else if(!strcmp(cmd, "reboot")) {
        reset(100);
    }
    else if(!strcmp(cmd, "loadimg")) {
        loadimg();
    }
    else if(!strcmp(cmd, "cpio")) {
        print_cpio();
    }
    else if(!strncmp(cmd, "alloc", 5)) {
        int size;
        char *p=cmd;
        char buf[100];
        int idx = 0;
        while(*p++!=' ');
        while(*p && *p!=' ') {
            buf[idx++] = *p++;
        }
        buf[idx] = '\0';
        size = atoi(buf);
        void *addr = kmalloc(size);
        uart_printf("addr: 0x%x, %d\n", addr, addr);
    }
    else if(!strncmp(cmd, "free", 4)) {
        char *p = cmd;
        while(*p++ != ' ');
        char buf[100];
        int idx=0;
        while(*p && *p!=' ') {
            buf[idx++] = *p++;
        }
        buf[idx] = '\0';
        void *addr = (void *)atoi(buf);
        kfree(addr); 
    }
    else if(!strncmp(cmd, "loaduser", 8)) {
        core_timer_enable();
        void *addr = load_program();
        void (*func)(void) = addr;
        uart_printf("prog_addr, %x\n", addr);
        el0_start(addr);
        func();
    }
    else if(!strcmp(cmd, "")) {
    }
    else {
        uart_putstr("Unknwon command!\n");
    }
}
