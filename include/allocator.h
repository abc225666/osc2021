#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#define PAGE_SIZE 0x1000
#define MAX_BUDDY_ORDER 9 // 2^0 ~ 2^8
#define MAX_POOL_PAGES 16
#define POOL_UNIT 16
#define POOL_SIZE 128 // 16 * 1 - 16 * 128(2048)

#include "typedef.h"

enum page_status {
    AVAIL,
    USED,
};

struct buddy_head {
    unsigned long nr_free;
    struct list_head head;
};

struct page_t {
    struct list_head list;
    int order;
    unsigned long idx;
    enum page_status status;
};

struct pool_t {
    int obj_size;
    int obj_per_page;
    int obj_used;
    int page_used;
    void *page_addr[MAX_POOL_PAGES];
    struct list_head free_list;
};

void mm_init();
void *kmalloc(unsigned long size);
void kfree(void *ptr);

void buddy_init();
void buddy_push(struct buddy_head *buddy, struct list_head *element);
void *buddy_alloc(int order);
void buddy_free(void *mem_addr);
void buddy_remove(struct buddy_head *buddy, struct list_head *element);
struct page_t *buddy_pop(struct buddy_head *buddy, int order);
unsigned long find_buddy(unsigned long offset, int order);

void *pool_alloc(unsigned long size);
void pool_free(unsigned int idx, void *addr);

void page_init();
void pool_init();
void mm_init();

#endif
