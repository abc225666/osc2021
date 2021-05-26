#include "queue.h"
#include "allocator.h"

struct queue *queue_init(unsigned long size) {
    struct queue *q = (struct queue *)kmalloc(sizeof(struct queue));
    q->buf = (char*)kmalloc(size);

    q->front = 0;
    q->rear = 0;
    q->size = size;

    return q;
}

int is_queue_empty(struct queue *q) {
    return q->front == q->rear;
}

int is_queue_full(struct queue *q) {
    return q->front == (q->rear+1)%q->size;
}

void queue_push(struct queue *q, char c) {
    // drop when queue is full
    if(is_queue_full(q)) {
        return;
    }

    q->buf[q->rear] = c;
    q->rear = (q->rear+1) % q->size;
}

char queue_pop(struct queue *q) {
    if(is_queue_empty(q)) {
        return '\0';
    }

    char r = q->buf[q->front];
    q->front = (q->front+1) % q->size;
    return r;
}
