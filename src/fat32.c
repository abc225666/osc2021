#include "fat32.h"
#include "vfs.h"
#include "allocator.h"
#include "mystring.h"
#include "sdhost.h"
#include "uart.h"

struct fat32_metadata fat32_metadata;

struct vnode_operations *fat32_v_ops;
struct file_operations *fat32_f_ops;

int fat32_setup_mount(struct filesystem *fs, struct mount *mount) {
    mount->fs = fs;
    mount->root = fat32_create_dentry(NULL, "/", DIRECTORY);
    return 0;
}

struct dentry *fat32_create_dentry(struct dentry *parent, const char *name, enum node_type type) {
    struct dentry *dentry = (struct dentry *)kmalloc(sizeof(struct dentry));
    memcpy(dentry->name, name, strlen(name));
    dentry->parent = parent;
    
    dentry->list.next = &dentry->list;
    dentry->list.prev = &dentry->list;

    dentry->childs.next = &dentry->childs;
    dentry->childs.prev = &dentry->childs;

    if(parent != NULL) {
        dentry->list.prev = parent->childs.prev;
        dentry->list.next = &parent->childs;

        parent->childs.prev->next = &dentry->list;
        parent->childs.prev = &dentry->list;
    }
    dentry->vnode = fat32_create_vnode(dentry);
    dentry->mountpoint = NULL;
    dentry->mount_parent = NULL;
    dentry->type = type;
    return dentry;
}

struct vnode *fat32_create_vnode(struct dentry *dentry) {
    struct vnode *vnode = (struct vnode *)kmalloc(sizeof(struct vnode));
    vnode->dentry = dentry;
    vnode->f_ops = fat32_f_ops;
    vnode->v_ops = fat32_v_ops;
    return vnode;
}

int fat32_register() {
    fat32_v_ops = (struct vnode_operations *)kmalloc(sizeof(struct vnode_operations));
    fat32_v_ops->lookup = fat32_lookup;
    fat32_v_ops->create = fat32_create;
    fat32_v_ops->mkdir = fat32_mkdir;
    fat32_v_ops->ls = fat32_ls;
    fat32_v_ops->load_dentry = fat32_load_dentry;

    fat32_f_ops = (struct file_operations *)kmalloc(sizeof(struct file_operations));
    fat32_f_ops->read = fat32_read;
    fat32_f_ops->write = fat32_write;
    return 0;
}

unsigned int get_cluster_blk_idx(unsigned int cluster_idx) {
    return fat32_metadata.data_region_blk_idx + (cluster_idx - fat32_metadata.first_cluster) * fat32_metadata.sector_per_cluster;
}

unsigned int get_fat_blk_idx(unsigned int cluster_idx) {
    return fat32_metadata.fat_region_blk_idx + (cluster_idx / FAT_ENTRY_PER_BLOCK);
}

unsigned int get_empty_fat_entry_idx(unsigned int current_idx) {
    unsigned int idx = current_idx+1;
    int fat[FAT_ENTRY_PER_BLOCK];

    int cur_tag = idx / FAT_ENTRY_PER_BLOCK;
    while(1) {
        readblock(get_fat_blk_idx(idx), fat);
        while(idx/FAT_ENTRY_PER_BLOCK == cur_tag) {
            if(fat[idx%FAT_ENTRY_PER_BLOCK] == 0) {
                return idx;
            }
            idx++;
        }
        cur_tag = idx / FAT_ENTRY_PER_BLOCK;
    }

    return 0;
}

int fat32_read(struct file *file, void *buf, unsigned long len) {
    struct fat32_internal *file_node = (struct fat32_internal *)file->vnode->internal;
    unsigned long f_pos_origin = file->f_pos;
    unsigned long current_cluster = file_node->first_cluster;
    int remain_len = len;
    int fat[FAT_ENTRY_PER_BLOCK];

    while(remain_len>0 && current_cluster >= fat32_metadata.first_cluster && current_cluster != EOC) {
        readblock(get_cluster_blk_idx(current_cluster), buf+file->f_pos-f_pos_origin);
        file->f_pos += (remain_len < BLOCK_SIZE) ? remain_len : BLOCK_SIZE;
        remain_len -= BLOCK_SIZE;

        if(remain_len > 0) {
            readblock(get_fat_blk_idx(current_cluster), fat);
            current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
        }
    }

    return file->f_pos-f_pos_origin;
}

int fat32_write(struct file *file, const void *buf, unsigned long len) {
    struct fat32_internal *file_node = (struct fat32_internal *)file->vnode->internal;
    unsigned long f_pos_origin = file->f_pos;
    unsigned long current_cluster = file_node->first_cluster;
    int remain_len = len;
    int fat[FAT_ENTRY_PER_BLOCK];
    char write_buf[BLOCK_SIZE];

    int remain_offset = file->f_pos;
    while(remain_offset > 0 && current_cluster >= fat32_metadata.first_cluster && current_cluster != EOC) {
        remain_offset -= BLOCK_SIZE;
        if(remain_offset > 0) {
            readblock(get_fat_blk_idx(current_cluster), fat);
            current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
        }
    }

    int buf_idx;
    int f_pos_offset = file->f_pos % BLOCK_SIZE;
    readblock(get_cluster_blk_idx(current_cluster), write_buf);
    for(buf_idx=0;buf_idx<BLOCK_SIZE-f_pos_offset&&buf_idx<len;buf_idx++) {
        write_buf[f_pos_offset + buf_idx] = ((char *)buf)[buf_idx];
    }
    writeblock(get_cluster_blk_idx(current_cluster), write_buf);
    file->f_pos += buf_idx;

    remain_len = len-buf_idx;
    while(remain_len>BLOCK_SIZE && current_cluster>=fat32_metadata.first_cluster && current_cluster != EOC) {
        writeblock(get_cluster_blk_idx(current_cluster), buf+buf_idx);
        file->f_pos += BLOCK_SIZE;
        remain_len -= BLOCK_SIZE;
        buf_idx += BLOCK_SIZE;

        readblock(get_fat_blk_idx(current_cluster), fat);
        current_cluster = fat[current_cluster % FAT_ENTRY_PER_BLOCK];
    }

    // last remain buf
    if(remain_len > 0) {
        readblock(get_cluster_blk_idx(current_cluster), write_buf);
        int idx;
        for(idx=0;remain_len>0;remain_len--,idx++) {
            write_buf[idx] = ((char *)buf)[buf_idx+idx];
        }
        writeblock(get_cluster_blk_idx(current_cluster), write_buf);
        file->f_pos += idx;
    }

    // update file size
    if(file->f_pos > file_node->size) {
        file_node->size = file->f_ops;

        unsigned char sector[BLOCK_SIZE];
        readblock(file_node->dirent_cluster, sector);
        struct fat32_dirent *sector_dirent = (struct fat32_dirent *)sector;
        for(int i=0;sector_dirent[i].name[0] != 0;i++) {
            if(sector_dirent[i].name[0] == 0xE5) {
                continue;
            }
            if( ((sector_dirent[i].cluster_high<<16)|(sector_dirent[i].cluster_low)) == file_node->first_cluster ) {
                sector_dirent[i].size = (unsigned int)file->f_pos;
            }
        }
        writeblock(file_node->dirent_cluster, sector);
    }

    return file->f_pos - f_pos_origin;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name) {
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
    *target = NULL;
    return -1;
}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    struct fat32_internal *dir_internal = (struct fat32_internal *)dir_node->internal;
    unsigned char sector[BLOCK_SIZE];
    unsigned int dirent_cluster = get_cluster_blk_idx(dir_internal->first_cluster);
    readblock(dirent_cluster, sector);

    char new_name[9];
    char new_ext[4];
    int idx = 0;
    while(idx<8 && component_name[idx] != '.' && component_name[idx] != 0) {
        new_name[idx] = component_name[idx];
        idx++;
    }
    new_name[idx] = '\0';
    while(component_name[idx] != '.') {
        idx++;
    }
    idx++;
    int idx2=0;
    while(idx2<3 && component_name[idx] != 0) {
        new_ext[idx2] = component_name[idx];
        idx2++;
        idx++;
    }
    new_ext[idx2] = '\0';

    uart_putstr(new_name);
    uart_putstr(new_ext);

    struct fat32_dirent *sector_dirent = (struct fat32_dirent *)sector;
    int i=0;
    while(sector_dirent[i].name[0]!=0) {
        i++;
    }


    // find empty fat entry
    unsigned int next_fat_idx = get_empty_fat_entry_idx(dir_internal->first_cluster);
    uart_printf("next: %d\n", next_fat_idx);

    // setup filename
    int len = strlen(new_name);
    if(len>8) len=8;
    memcpy(sector_dirent[i].name, new_name, len);
    for(int j=len;j<8;j++) {
        sector_dirent[i].name[j] = ' ';
    }
    len = strlen(new_ext);
    if(len>3) len=3;
    memcpy(sector_dirent[i].ext, new_ext, len);
    for(int j=len;j<3;j++) {
        sector_dirent[i].name[j] = ' ';
    }

    sector_dirent[i].attr = 0x20;
    // setup idx to cluster_high and cluster_low
    sector_dirent[i].cluster_high = (unsigned short)(next_fat_idx >> 16);
    sector_dirent[i].cluster_low = (unsigned short)(next_fat_idx & 0xFFFF);
    sector_dirent[i].size = 0;

    // setup first fat entry to EOC
    int fat[FAT_ENTRY_PER_BLOCK];
    readblock(get_fat_blk_idx(next_fat_idx), fat);
    fat[next_fat_idx % FAT_ENTRY_PER_BLOCK] = EOC;
    writeblock(get_fat_blk_idx(next_fat_idx), fat);



    // write back
    writeblock(dirent_cluster, sector);

    // get filename to create vnode
    char filename[13];
    idx=0;
    for(int j=0;j<8;j++) {
        char c = sector_dirent[i].name[j];
        if(c==' ') {
            break;
        }
        filename[idx++] = c;
    }
    filename[idx++] = '.';
    for(int j=0;j<3;j++) {
        char c = sector_dirent[i].ext[j];
        if(c==' ') {
            break;
        }
        filename[idx++] = c;
    }
    filename[len++] = '\0';
    // add new dentry
    struct dentry *new_dentry;
    new_dentry = fat32_create_dentry(dir_node->dentry, filename, REGULAR_FILE);
    struct fat32_internal *child_internal = (struct fat32_internal *)kmalloc(sizeof(struct fat32_internal));
    child_internal->first_cluster = next_fat_idx;
    child_internal->dirent_cluster = dirent_cluster;
    child_internal->size = sector_dirent[i].size;
    new_dentry->vnode->internal = child_internal;

    *target = new_dentry->vnode;

    return 0;
}

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    return 0;
}

int fat32_ls(struct vnode *vnode) {
    return 0;
}


int fat32_load_dentry(struct dentry *dentry, const char *component_name) {
    struct fat32_internal *dir_internal = (struct fat32_internal *)dentry->vnode->internal;
    unsigned char sector[BLOCK_SIZE];
    unsigned int dirent_cluster  = get_cluster_blk_idx(dir_internal->first_cluster);
    readblock(dirent_cluster, sector);
    
    struct fat32_dirent *sector_dirent = (struct fat32_dirent *)sector;
    int found = -1;

    for(int i=0;sector_dirent[i].name[0] != 0;i++) {
        if(sector_dirent[i].name[0] == 0xE5) {
            continue;
        }

        char filename[13];
        int len=0;
        for(int j=0;j<8;j++) {
            char c = sector_dirent[i].name[j];
            if(c==' ') {
                break;
            }
            filename[len++] = c;
        }
        filename[len++] = '.';
        for(int j=0;j<3;j++) {
            char c = sector_dirent[i].ext[j];
            if(c==' ') {
                break;
            }
            filename[len++] = c;
        }
        filename[len++] = '\0';


        if(!strcmp(filename, component_name)) {
            found = 0;
        }
        
        struct dentry *new_dentry;
        if(sector_dirent[i].attr == 0x10) {
            new_dentry = fat32_create_dentry(dentry, filename, DIRECTORY);
        }
        else {
            new_dentry = fat32_create_dentry(dentry, filename, REGULAR_FILE);
        }

        struct fat32_internal *child_internal = (struct fat32_internal *)kmalloc(sizeof(struct fat32_internal));
        child_internal->first_cluster = ((sector_dirent[i].cluster_high<<16) | (sector_dirent[i].cluster_low));
        child_internal->dirent_cluster = dirent_cluster;
        child_internal->size = sector_dirent[i].size;
        new_dentry->vnode->internal = child_internal;

        async_printf("i: %d, first: %d\n", i, child_internal->first_cluster);
        async_putstr(filename);
        async_putstr("\n");
    }

    return found;
}
