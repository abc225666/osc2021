#include "allocator.h"
#include "cpio.h"
#include "mmio.h"
#include "typedef.h"
#include "uart.h"
#include "mbox.h"

extern unsigned char __end;


struct buddy_head buddy_order[MAX_BUDDY_ORDER];
struct pool_t obj_pool[POOL_SIZE];
struct page_t *page;

void buddy_init() {
    for(int i=0;i<MAX_BUDDY_ORDER;i++) {
        buddy_order[i].nr_free = 0;
        buddy_order[i].head.next = &buddy_order[i].head;
        buddy_order[i].head.prev = &buddy_order[i].head;
    }
}

void buddy_push(struct buddy_head *buddy, struct list_head *element) {
    buddy->nr_free++;
    element->next = &(buddy->head);
    element->prev = buddy->head.prev;
    buddy->head.prev->next = element;
    buddy->head.prev = element;
}

struct page_t *buddy_pop(struct buddy_head *buddy, int order) {
    if(buddy->head.next == &buddy->head) return NULL;
    buddy->nr_free--;
    // get target
    struct list_head *target = buddy->head.next;

    // remove from list;
    target->next->prev = target->prev;
    target->prev->next = target->next;
    target->next = NULL;
    target->prev = NULL;

    // release redundant page
    struct page_t *target_page = (struct page_t *)target;
    int cur_order = target_page->order;

    while(cur_order > order) {
        //uart_printf("split order %d to %d\n", cur_order, cur_order-1);
        cur_order--;
        struct page_t *left = target_page;
        struct page_t *right = target_page + (1<<cur_order);
        left->order = cur_order;
        right->order = cur_order;
        buddy_push(&buddy_order[cur_order], &(right->list));
    }

    for(int i=0;i<(1<<order);i++) {
        target_page[i].status = USED;
    }
    return target_page;
}

void buddy_remove(struct buddy_head *buddy, struct list_head *element) {
    buddy->nr_free--;
    element->prev->next = element->next;
    element->next->prev = element->prev;
    element->prev = NULL;
    element->next = NULL;
}

void *buddy_alloc(int order) {
    // find free space
    for(int i=order;i<MAX_BUDDY_ORDER;i++) {
        if(buddy_order[i].nr_free>0) {
            struct page_t *target = buddy_pop(&buddy_order[i], order);
            return (void *)(target->idx * PAGE_SIZE);
        }
    }
    return NULL;
}

void buddy_free(void *mem_addr) {
    unsigned long page_idx = (unsigned long)mem_addr / PAGE_SIZE;
    struct page_t *p = &page[page_idx];
    int order = p->order;
    unsigned long buddy_idx = find_buddy(page_idx, order);
    struct page_t *buddy_p = &page[buddy_idx];


    // merge buddy
    while(buddy_p->status == AVAIL && buddy_p->order == order) {
        buddy_remove(&buddy_order[order], &(buddy_p->list));
        //uart_printf("merge buddy order %d to %d\n", order, order+1);

        order++;
        if(page_idx < buddy_idx) {
            buddy_p->order = -1;
        }
        else {
            p->order = -1;
            p = buddy_p;
            page_idx = buddy_idx;
        }
        buddy_idx = find_buddy(page_idx, order);
        buddy_p = &page[buddy_idx];
    }

    for(int i=0;i<(1<<order);i++) {
        (p+i)->status = AVAIL;
        p->order = order;
        buddy_push(&buddy_order[order], &(p->list));
    }
}

unsigned long find_buddy(unsigned long offset, int order) {
    unsigned long buddy_offset = offset ^ (1<<order);
    return buddy_offset;
}

void pool_init() {
    for(int i=0;i<POOL_SIZE;i++) {
        obj_pool[i].obj_size = POOL_UNIT * (i+1);
        obj_pool[i].obj_per_page = PAGE_SIZE / obj_pool[i].obj_size;
        obj_pool[i].obj_used = 0;
        obj_pool[i].page_used = 0;
        obj_pool[i].free_list.next = &obj_pool[i].free_list;
        obj_pool[i].free_list.prev= &obj_pool[i].free_list;
    }
}

void *pool_alloc(unsigned long size) {
    if(size==0) return NULL;

    unsigned long align_size = align_up(size, POOL_UNIT);
    unsigned int pool_idx = align_size / POOL_UNIT - 1;

    struct pool_t *pool = &obj_pool[pool_idx];
    //uart_printf("alloc from pool size: %d\n", (pool_idx + 1) * POOL_UNIT);
    // take from pool free list
    if(pool->free_list.next != &pool->free_list) {
        //uart_printf("used free block\n");
        struct list_head *element = (struct list_head *)pool->free_list.next;
        element->prev->next = element->next;
        element->next->prev = element->prev;
        element->next = NULL;
        element->prev = NULL;
        return (void *)element;
    }
    // out of page, alloc new page
    if(pool->obj_used >= pool->page_used * pool->obj_per_page) {
        //uart_printf("request from buddy system\n");
        pool->page_addr[pool->page_used] = buddy_alloc(0);
        pool->page_used++;
    }
    // alloc new obj
    void *addr = pool->page_addr[pool->page_used-1] +
            (pool->obj_used % pool->obj_per_page) * pool->obj_size;
    pool->obj_used++;
    return addr;
}

void page_init() {
    //uart_printf("end: %x\n", arm_memory_end);
    unsigned long first_avail_page = (unsigned long)&__end / PAGE_SIZE + 1;
    unsigned long end_page = arm_memory_end / PAGE_SIZE;
    //uart_printf("first avai: %d\n", first_avail_page);
    //uart_printf("%d\n", end_page);

    unsigned long bytes = sizeof(struct page_t) * end_page;
    unsigned long ds_page = align_up(bytes, PAGE_SIZE) / PAGE_SIZE;

    void *page_addr = (void *)(first_avail_page * PAGE_SIZE);
    page = (struct page_t *) page_addr;
    first_avail_page += ds_page;

    for(int i=0;i<first_avail_page;i++) {
        page[i].status = USED;
        page[i].idx = i;
    }


    int remainder = first_avail_page % (1<< (MAX_BUDDY_ORDER-1));
    int order = MAX_BUDDY_ORDER - 1;
    int idx = first_avail_page + (1<<(MAX_BUDDY_ORDER-1)) + remainder;
    int counter = 0;
    while(idx<end_page) {
        if(counter) {
            page[idx].status = AVAIL;
            page[idx].order = -1;
            page[idx].list.next = NULL;
            page[idx].list.prev = NULL;
            page[idx].idx = idx;
            counter--;
            idx++;
        }
        else if(idx + (1<<order)-1 < end_page) {
            //uart_printf("order: %d, idx: %d, addr: %x, phy: %x\n", order, idx, &page[idx].list, idx*PAGE_SIZE);
            page[idx].status = AVAIL;
            page[idx].order = order;
            page[idx].idx = idx;
            buddy_push(&(buddy_order[order]), &(page[idx].list));
            counter = (1<<order) - 1;
            idx++;
        }
        else {
            order--;
        }
    }
}

void pool_free(unsigned int idx, void *addr) {
    struct pool_t *pool = &obj_pool[idx];

    struct list_head *free_addr = (struct list_head *)addr;
    free_addr->prev = pool->free_list.prev;
    free_addr->next = &pool->free_list;

    pool->free_list.prev->next = free_addr;
    pool->free_list.prev = free_addr;

    pool->obj_used--;
}

void mm_init() {
    buddy_init();
    page_init();
    pool_init();
}

void *kmalloc(unsigned long size) {
    if(size > POOL_UNIT*POOL_SIZE) {
        // find order
        int order;
        for(int i=0;i<MAX_BUDDY_ORDER;i++) {
            if(size <= (unsigned long)(1<<i)*PAGE_SIZE) {
                order = i;
                break;
            }
        }
        return buddy_alloc(order);
    }
    else {
        //uart_printf("malloc from pool\n");
        return pool_alloc(size);
    }
}

void kfree(void *ptr) {
    // try free from pool
    for(int i=0;i<POOL_SIZE;i++) {
        for(int j=0;j<obj_pool[i].page_used;i++) {
            unsigned long page_base = (unsigned long)obj_pool[i].page_addr[j];
            unsigned long align_addr = align_down((unsigned long)ptr, PAGE_SIZE);
            if(page_base == align_addr) {
                //uart_printf("free from pool: 0x%x\n", ptr);
                pool_free(i, ptr);
                return;
            }
        }
    }
    //uart_printf("free from buddy system\n");
    buddy_free(ptr);
}

