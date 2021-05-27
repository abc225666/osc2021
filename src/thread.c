#include "thread.h"
#include "allocator.h"
#include "uart.h"

struct thread_head *thread_pool;

int global_tid = 1;

void thread_pool_init() {
    thread_pool = (struct thread_head *)kmalloc(sizeof(struct thread_head));

    thread_pool->num_threads = 0;
    thread_pool->thread_list.next = &thread_pool->thread_list;
    thread_pool->thread_list.prev = &thread_pool->thread_list;
}

void thread_create(void(*func)()) {
    struct thread_t *t = (struct thread_t *)kmalloc(sizeof(struct thread_t));

    // setup thread info
    t->tid = global_tid++;
    t->state = RUNNING;


    // allocate require memory
    void *sp =  kmalloc(KSTACK_SIZE) + KSTACK_SIZE - 16;

    // set program address
    t->context.lr = (unsigned long)func;
    t->context.fp = (unsigned long)sp;
    t->context.sp = (unsigned long)sp;

    async_printf("create\n");

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

void schedule() {
    struct thread_t *cur_thread = get_current_thread();
    if(cur_thread->state == RUNNING) {
        thread_push(cur_thread);
    }
    struct thread_t *next_thread = thread_pop();
    async_printf("tid %d\n", next_thread->tid);
    context_switch(cur_thread, next_thread);
}


void context_switch(struct thread_t *cur_thread, struct thread_t *next_thread) {
    update_current_thread(next_thread);
    switch_to(&cur_thread->context, &next_thread->context);
}
