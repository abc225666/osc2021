#include "thread.h"
#include "allocator.h"
#include "uart.h"
#include "process.h"
#include "mystring.h"
#include "syscall.h"

struct thread_head *thread_pool;

int global_tid = 1;

int new_pid() {
    global_tid++;
    return global_tid-1;
}

void thread_pool_init() {
    thread_pool = (struct thread_head *)kmalloc(sizeof(struct thread_head));

    thread_pool->num_threads = 0;
    thread_pool->thread_list.next = &thread_pool->thread_list;
    thread_pool->thread_list.prev = &thread_pool->thread_list;

    uart_printf("thread_pool: %x\n", thread_pool);
}

void thread_create(void *func, int argc, char *argv[]) {
    struct thread_t *t = (struct thread_t *)kmalloc(sizeof(struct thread_t));

    // setup thread info
    t->tid = global_tid++;
    t->state = RUNNING;


    // allocate require memory
    void *sp_base =  kmalloc(KSTACK_SIZE);
    void *user_sp_base = kmalloc(KSTACK_SIZE);

    // set program address
    t->fptr = func;
    t->sp_base = sp_base;
    t->user_sp_base = user_sp_base;
    t->context.lr = (unsigned long)exit_wrapper;
    t->context.fp = (unsigned long)sp_base + KSTACK_SIZE;
    t->context.sp = (unsigned long)sp_base + KSTACK_SIZE;

    async_printf("create, sp: %x\n", sp_base);

    t->argc = argc;
    void *user_sp = user_sp_base + KSTACK_SIZE;
    for(int i=argc-1;i>=0;i--) {
        user_sp -= sizeof(argv[i]);
        *(char **)user_sp = argv[i];
    }
    t->user_sp = user_sp;

    thread_push(t);
}

void thread_push(struct thread_t *thread) {
    thread->list.prev = thread_pool->thread_list.prev;
    thread->list.next = &thread_pool->thread_list;

    thread_pool->thread_list.prev->next = &thread->list;
    thread_pool->thread_list.prev = &thread->list;
    thread_pool->num_threads++;
}

struct thread_t *thread_pop() {
    struct thread_t *res = (struct thread_t *)(thread_pool->thread_list.next);

    res->list.next->prev = res->list.prev;
    res->list.prev->next = res->list.next;
    res->list.next = NULL;
    res->list.prev = NULL;
    return res;
}

void do_schedule() {
    struct thread_t *cur_thread = get_current_thread();
    if(cur_thread->state == RUNNING) {
        thread_push(cur_thread);
    }
    struct thread_t *next_thread = thread_pop();
    //async_printf("tid %d\n", next_thread->tid);
    update_current_thread(next_thread);
    switch_to(&cur_thread->context, &next_thread->context);
}

void exit_wrapper() {
    struct thread_t *thread = get_current_thread();
    thread->fptr(thread->argc, thread->user_sp);
    thread = get_current_thread();
    thread->state = ZOMBIE;
    kfree(thread->sp_base);
    kfree(thread->user_sp_base);
    do_schedule();
}


void context_switch(struct thread_t *cur_thread, struct thread_t *next_thread) {
    update_current_thread(next_thread);
    switch_to(&cur_thread->context, &next_thread->context);
}
