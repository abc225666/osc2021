#ifndef THREAD_H
#define THREAD_H

#include "typedef.h"

#define KSTACK_SIZE 4096

typedef enum {
    RUNNING,
    WAITING,
    ZOMBIE,
    DEAD
} THREAD_STATE;

struct thread_head {
    int num_threads;
    struct list_head thread_list;
};

struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long lr;
    unsigned long sp;
};

struct thread_t {
    struct list_head list;
    int tid;
    void (*fptr)();
    void *sp_base;
    void *user_sp_base;
    void *user_sp;
    int argc;
    THREAD_STATE state;
    struct cpu_context context;
};

extern void switch_to(struct cpu_context *prev, struct cpu_context *next);
extern void update_current_thread(struct thread_t *t);
extern struct thread_t *get_current_thread();

void thread_pool_init();
void thread_create(void *func, int argc, char *argv[]);
void thread_push(struct thread_t *thread);
struct thread_t *thread_pop();

void schedule();
void context_switch(struct thread_t *cur_thread, struct thread_t *next_thread);
void exit_wrapper();

#endif
