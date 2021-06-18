#include "process.h"
#include "thread.h"
#include "uart.h"

void do_exit() {
    async_printf("exit!!");
}
