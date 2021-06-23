#include "tmpfs.h"
#include "vfs.h"
#include "mystring.h"
#include "allocator.h"
#include "uart.h"

struct vnode_operations *tmpfs_v_ops;
struct file_operations *tmpfs_f_ops;

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount) {
    mount->fs = fs;
    mount->root = tmpfs_create_dentry(NULL, "/", DIRECTORY);
    return 0;
}

struct dentry *tmpfs_create_dentry(struct dentry *parent, const char *name, enum node_type type) {
    struct dentry *dentry = (struct dentry *)kmalloc(sizeof(struct dentry));
    memcpy(dentry->name, name, strlen(name));
    dentry->parent = parent;

    // init list head
    dentry->list.next = &dentry->list;
    dentry->list.prev = &dentry->list;

    dentry->childs.next = &dentry->childs;
    dentry->childs.prev = &dentry->childs;

    // add list to parent's childs
    if(parent != NULL) {
        dentry->list.prev = parent->childs.prev;
        dentry->list.next = &parent->childs;

        parent->childs.prev->next = &dentry->list;
        parent->childs.prev = &dentry->list;
    }

    dentry->vnode = tmpfs_create_vnode(dentry);
    dentry->mountpoint = NULL;
    dentry->type = type;
    return dentry;
}

struct vnode *tmpfs_create_vnode(struct dentry *dentry) {
    struct vnode *vnode = (struct vnode *)kmalloc(sizeof(struct vnode));
    vnode->dentry = dentry;
    
    // connect f_ops vops
    vnode->f_ops = tmpfs_f_ops;
    vnode->v_ops = tmpfs_v_ops;
    return vnode;
}

int tmpfs_register() {
    tmpfs_v_ops = (struct vnode_operations *)kmalloc(sizeof(struct vnode_operations));
    tmpfs_v_ops->lookup = tmpfs_lookup;
    tmpfs_v_ops->create = tmpfs_create;

    tmpfs_f_ops = (struct file_operations *)kmalloc(sizeof(struct file_operations));
    tmpfs_f_ops->read = tmpfs_read;
    tmpfs_f_ops->write = tmpfs_write;

    return 0;
}

int tmpfs_write(struct file *file, const void *buf, unsigned long len) {
    struct tmpfs_internal *file_node = (struct tmpfs_internal *)file->vnode->internal;

    char *dest = &file_node->buf[file->f_pos];
    char *src = (char *)buf;
    unsigned long i = 0;
    for(;i<len&&src[i]!='\0'&&file->f_pos+i<TMP_FILE_SIZE-1;i++) {
        dest[i] = src[i];
    }
    dest[i] = EOF;
    return i;
}

int tmpfs_read(struct file *file, void* buf, unsigned long len) {
    struct tmpfs_internal *file_node = (struct tmpfs_internal *)file->vnode->internal;

    char *dest = (char *)buf;
    char *src = &file_node->buf[file->f_pos];
    unsigned long i = 0;
    for(;i<len && src[i] != (unsigned char)EOF;i++) {
        dest[i] = src[i];
    }
    return i;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    if(!strcmp(component_name, "")) {
        *target = dir_node;
        return 0;
    }

    struct list_head *p;
    for(p=dir_node->dentry->childs.next;p!=&dir_node->dentry->childs;p=p->next) {
        struct dentry *dentry = list_entry(p, struct dentry, list);
        if(!strcmp(dentry->name, component_name)) {
            *target = dentry->vnode;
            return 0;
        }
    }
    return -1;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target , const char *component_name) {
    struct tmpfs_internal *file_node = (struct tmpfs_internal *)kmalloc(sizeof(struct tmpfs_internal));

    struct dentry *d_child = tmpfs_create_dentry(dir_node->dentry, component_name, REGULAR_FILE);
    d_child->vnode->internal = (void *)file_node;

    *target = d_child->vnode;
    return 0;
}
