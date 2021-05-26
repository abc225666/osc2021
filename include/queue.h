#ifndef QUEUE_H
#define QUEUE_H

struct queue {
    int front;
    int rear;
    unsigned long size;
    char *buf;
};

struct queue * queue_init(unsigned long size);
int is_queue_empty(struct queue *q);
int is_queue_full(struct queue *q);
void queue_push(struct queue *q, char c);
char queue_pop(struct queue *q);

#endif
