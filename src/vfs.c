#include "vfs.h"
#include "allocator.h"
#include "mystring.h"
#include "uart.h"
#include "tmpfs.h"
#include "thread.h"
#include "typedef.h"


void rootfs_init() {
    struct filesystem *tmpfs = (struct filesystem *)kmalloc(sizeof(struct filesystem));
    tmpfs->name = (char *)kmalloc(sizeof(char)*6);
    memcpy(tmpfs->name, "tmpfs", 6);
    // tmpfs setup point
    tmpfs->setup_mount = tmpfs_setup_mount;

    register_filesystem(tmpfs);

    rootfs = (struct mount *)kmalloc(sizeof(struct mount));
    // call mount
    tmpfs->setup_mount(tmpfs, rootfs);
    
}

int register_filesystem(struct filesystem *fs) {
    if(!strcmp(fs->name, "tmpfs")) {
        async_printf("Register tmpfs\n"); 
        return tmpfs_register();
    }
    return 0;
}

struct file *vfs_open(const char *pathname, int flags) {
    // find target dir;
    struct vnode *target_dir;
    char target_path[128];
    traversal(pathname, &target_dir, target_path);

    // create file fd when found
    // create new file when flag has create
    struct vnode *target_file;
    if(target_dir->v_ops->lookup(target_dir, &target_file, target_path)==0) {
        return create_fd(target_file);
    }
    else {
        if(flags & O_CREAT) {
            int res = target_dir->v_ops->create(target_dir, &target_file, target_path);
            if(res < 0) return NULL;
            target_file->dentry->type = REGULAR_FILE;
            return create_fd(target_file);
        }
        else {
            return NULL;
        }
    }
    return NULL;
}

int vfs_close(struct file *file) {
    kfree((void *)file);
    return 0;
}

int vfs_read(struct file *file, void *buf, unsigned long len) {
    if(file->vnode->dentry->type != REGULAR_FILE) {
        async_printf("Read on non-regular file\n");
        return -1;
    }
    return file->f_ops->read(file, buf, len);
}

int vfs_write(struct file *file, const void *buf, unsigned long len) {
    if(file->vnode->dentry->type != REGULAR_FILE) {
        async_printf("Write on non-regular file\n");
        return -1;
    }
    return file->f_ops->write(file, buf, len);
}

void list_dir(const char *pathname) {
    struct vnode *target_dir;
    char target_path[128];
    traversal(pathname, &target_dir, target_path);

    struct list_head *p;
    for(p=target_dir->dentry->childs.next;p!=&target_dir->dentry->childs;p=p->next) {
        struct dentry *entry = list_entry(p, struct dentry, list);
        async_printf("Name: ");
        async_putstr(entry->name);
        async_printf("\n");
    }

}

void traversal(const char *pathname, struct vnode **target_node, char *target_path) {
    // absolute path start with /
    if(pathname[0] == '/') {
        struct vnode *rootnode = rootfs->root->vnode;
        traversal_recursive(rootnode->dentry, pathname+1, target_node, target_path);
    }
    // relative path, start from pwd
    else {
        struct vnode *rootnode = get_current_thread()->pwd->vnode;
        traversal_recursive(rootnode->dentry, pathname, target_node, target_path);
    }
}

void traversal_recursive(struct dentry *node, const char *path, struct vnode **target_node, char *target_path) {
    int i = 0;
    while(path[i]) {
        if(path[i] == '/') {
            // new layer
            break;
        }
        target_path[i] = path[i];
        i++;
    }
    target_path[i++] = '\0';
    *target_node = node->vnode;
    if(!strcmp(target_path, "")) {
        // end recur, notice two slash may also enter here
        return;
    }
    // find child
    struct list_head *p;
    for(p=(&node->childs)->next;p!=&node->childs;p=p->next) {
        struct dentry *entry = list_entry(p, struct dentry, list);
        if(!strcmp(entry->name, target_path)) {
            if(entry->type == DIRECTORY) {
                traversal_recursive(entry, path+i, target_node, target_path);
            }
            break;
        }
    }
}

struct file *create_fd(struct vnode *target) {
    struct file *fd = (struct file *)kmalloc(sizeof(struct file));
    fd->f_ops = target->f_ops;
    fd->vnode = target;
    fd->f_pos = 0;
    return fd;
}
