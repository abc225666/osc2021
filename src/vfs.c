#include "vfs.h"
#include "allocator.h"
#include "mystring.h"
#include "uart.h"
#include "tmpfs.h"
#include "fat32.h"
#include "thread.h"
#include "typedef.h"


void fs_init() {
    tmpfs.name = (char *)kmalloc(sizeof(char)*6);
    memcpy(tmpfs.name, "tmpfs", 6);
    tmpfs.setup_mount = tmpfs_setup_mount;
    register_filesystem(&tmpfs);

    fat32.name = (char *)kmalloc(sizeof(char)*6);
    memcpy(fat32.name, "fat32", 6);
    fat32.setup_mount = fat32_setup_mount;
    register_filesystem(&fat32);
}

void rootfs_init() {
    rootfs = (struct mount *)kmalloc(sizeof(struct mount));
    // call mount
    tmpfs.setup_mount(&tmpfs, rootfs);
}

int register_filesystem(struct filesystem *fs) {
    if(!strcmp(fs->name, "tmpfs")) {
        async_printf("Register tmpfs\n"); 
        return tmpfs_register();
    }
    else if(!strcmp(fs->name, "fat32")) {
        async_printf("Register fat32\n");
        return fat32_register();
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

int vfs_ls(const char *pathname) {
    struct vnode *target_dir;
    char target_path[128];
    traversal(pathname, &target_dir, target_path);

    return target_dir->v_ops->ls(target_dir);

}

int vfs_mkdir(const char *pathname) {
    struct vnode *target_dir;

    char newdir_name[128];
    // last node is new node
    traversal(pathname, &target_dir, newdir_name);
    struct vnode *child_dir;
    int res = target_dir->v_ops->mkdir(target_dir, &child_dir, newdir_name);
    if(res<0) return res; // error
    child_dir->dentry->type = DIRECTORY;
    return 0;
}

int vfs_chdir(const char *pathname) {
    struct thread_t *cur_thread = get_current_thread();

    struct vnode *target_dir;
    char path_remain[128];
    traversal(pathname, &target_dir, path_remain);

    if(strcmp(path_remain, "")) { // node left, not exist
        return -1;
    }
    else {
        cur_thread->pwd = target_dir->dentry;
        return 0;
    }
}

int vfs_mount(const char *device, const char *mountpoint, const char *filesystem) {
    struct vnode *mount_dir;
    char path_remain[128];
    traversal(mountpoint, &mount_dir, path_remain);

    // should mount on a directory
    if(!strcmp(path_remain, "")) { // path exist
        if(mount_dir->dentry->type != DIRECTORY) {
            async_printf("mount fail 1\n");
            return -1;
        }
    }
    else { // path not exist
        async_printf("mount fail 2\n");
        return -2;
    }

    struct mount *new_mount = (struct mount *)kmalloc(sizeof(struct mount));
    if(!strcmp(filesystem, "tmpfs")) {
        tmpfs.setup_mount(&tmpfs, new_mount);
    }
    else if(!strcmp(filesystem, "fat32")) {
        fat32.setup_mount(&fat32, new_mount);
    }
    mount_dir->dentry->mountpoint = new_mount;
    new_mount->root->mount_parent = mount_dir->dentry;
    return 0;
}

int vfs_umount(const char *mountpoint) {
    struct vnode *mount_dir;
    char path_remain[128];
    traversal(mountpoint, &mount_dir, path_remain);
    if(!strcmp(path_remain, "")) {
        if(mount_dir->dentry->mount_parent == NULL) {
            async_printf("unmount fail 1\n");
            return -1;
        } 
    }
    else {
        // no mountpoint here
        async_printf("unmount fail 2\n");
        return -2;
    }

    // TODO: should recursive free dentry

    mount_dir->dentry->mount_parent->mountpoint = NULL;

    return 0;
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
    target_path[i] = '\0';
    if(path[i]!='\0') {
        i++;
    }


    *target_node = node->vnode;

    if(!strcmp(target_path, "")) {
        // end recur, notice two slash may also enter here
        return;
    }
    // find child
    struct list_head *p;
    int found = 0;
    for(p=(&node->childs)->next;p!=&node->childs;p=p->next) {
        struct dentry *entry = list_entry(p, struct dentry, list);
        if(!strcmp(entry->name, target_path)) {
            if(entry->mountpoint != NULL) { // something mount on
                traversal_recursive(entry->mountpoint->root, path+i, target_node, target_path);
            }
            else if(entry->type == DIRECTORY) {
                traversal_recursive(entry, path+i, target_node, target_path);
            }
            found = 1;
            break;
        }
    }
    if(!found) {
        int ret = node->vnode->v_ops->load_dentry(node, target_path);
        if(ret==0) {
            traversal_recursive(node, path, target_node, target_path);
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
