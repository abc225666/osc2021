#ifndef TYPEDEF_H
#define TYPEDEF_H

#define NULL ((void *)0)

#define offsetof(TYPE, MEMBER) ((unsigned long long)&((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type, member) ); });

#define list_entry(ptr, type, member) container_of(ptr, type, member)

struct list_head {
    struct list_head *next, *prev;
};

#endif
