#ifndef VFS_H
#define VFS_H

#define O_CREAT 1

#include "typedef.h"

enum node_type {
    DIRECTORY,
    REGULAR_FILE
};

struct vnode {
    struct dentry *dentry;
    struct vnode_operations* v_ops;
    struct file_operations* f_ops;
    void* internal;
};

struct file {
    struct vnode* vnode;
    unsigned long f_pos; // The next read/write position of this opened file
    struct file_operations* f_ops;
    int flags;
};

struct mount {
    struct dentry* root;
    struct filesystem* fs;
};

struct dentry {
    char name[32];
    struct dentry *parent;
    struct vnode *vnode;
    struct list_head list;
    struct list_head childs;
    enum node_type type;
    struct mount *mountpoint;
    struct dentry *mount_parent;
};

struct filesystem {
    char* name;
    int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
    int (*write) (struct file* file, const void* buf, unsigned long len);
    int (*read) (struct file* file, void* buf, unsigned long len);
};

struct vnode_operations {
    int (*lookup)(struct vnode* dir_node, struct vnode** target, const char* component_name);
    int (*create)(struct vnode* dir_node, struct vnode** target, const char* component_name);
};

struct file *vfs_open(const char *pathname, int flags);
int vfs_close(struct file *file);
int vfs_read(struct file *file, void *buf, unsigned long len);
int vfs_write(struct file *file, const void *buf, unsigned long len);

void traversal(const char *pathname, struct vnode **target_node, char *target_path);
void traversal_recursive(struct dentry *node, const char *path, struct vnode **target_node, char *target_path);
struct file *create_fd(struct vnode *target);

void rootfs_init();
int register_filesystem(struct filesystem *fs);

struct mount *rootfs;

void list_dir(const char *pathname);


#endif
