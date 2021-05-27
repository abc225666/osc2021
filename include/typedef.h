#ifndef TYPEDEF_H
#define TYPEDEF_H

#define NULL ((void *)0)

struct list_head {
    struct list_head *next, *prev;
};

#endif
