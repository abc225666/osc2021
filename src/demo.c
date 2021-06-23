#include "demo.h"
#include "uart.h"
#include "mystring.h"
#include "shell.h"
#include "thread.h"
#include "syscall.h"
#include "vfs.h"

#define CMDSIZE 128

void shell() {
    char cmd[CMDSIZE] = {0};
    int cmd_idx = 0;
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
        do_schedule();
    }
}

void fork_test(int argc, char **argv) {
    async_printf("Fork Test, pid %d\n", getpid());
    int cnt = 1;
    int ret = fork();
    if(ret==0) {
        async_printf("pid: %d, cnt: %d, ptr: %x\n", getpid(), cnt, &cnt);
        ++cnt;
        fork();
        while(cnt<5) {
            async_printf("pid: %d, cnt: %d, ptr: %x\n", getpid(), cnt, &cnt);
            ++cnt;
        }
    }
    else {
        async_printf("parent heare, pid %d, child %d\n", getpid(), ret);
    }
}

void argv_test(int argc, char **argv) {
    async_printf("Argv Test, pid %d\n", getpid());
    for(int i=0;i<argc;i++) {
        async_putstr(argv[i]);
        async_putstr("\n");
    }

    char *fork_argv[] = {"fork_test", 0};
    exec(fork_test, fork_argv);
}

void demo_0() {
    char *argv[] = {"argv_test", "-o", "arg2", 0};
    exec(argv_test, argv);
}

void demo_vfs() {
    char buf[8]; 
    int a = open("hello", O_CREAT);
    int b = open("world", O_CREAT);

    async_printf("%d %d\n", a, b);

    write(a, "Hello", 6);
    write(b, "World!", 6);
    close(a);
    close(b);

    a = open("hello", 0);
    b = open("world", 0);
    int sz;
    sz = read(a, buf, 100);
    sz += read(b, buf+sz, 100);
    buf[sz] = '\0';
    async_putstr(buf);
    async_printf("\n", buf);
    list_dir("/");
}
