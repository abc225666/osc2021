#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

#define TMP_FILE_SIZE 512

struct tmpfs_internal {
    char buf[512];
};

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount);
struct dentry *tmpfs_create_dentry(struct dentry *parent, const char *name, enum node_type type);
struct vnode *tmpfs_create_vnode(struct dentry *dentry);
int tmpfs_register();

// f_opts
int tmpfs_write(struct file *file, const void *buf, unsigned long len);
int tmpfs_read(struct file *file, void *buf, unsigned long len);

// v_opts
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_ls(struct vnode *dir_node);
int tmpfs_load_dentry(struct dentry *dentry, const char *component_name);


#endif
