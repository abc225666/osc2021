#ifndef FAT32_H
#define FAT32_H

#include "vfs.h"

#define BLOCK_SIZE 512
#define FAT_ENTRY_PER_BLOCK (BLOCK_SIZE/sizeof(int))
#define EOC 0xFFFFFFFF

struct fat32_boot_sector {
    char jump[3]; // 0x0
    char oem[8]; // 0x3

    // fat32 extend BIOS

    // BIOS parameter block
    unsigned short bytes_per_logical_sector; // 0xb
    unsigned char logical_sector_per_cluster; // 0xd
    unsigned short n_reserved_sectors; // 0xe
    unsigned char n_file_alloc_tabs; // 0x10
    unsigned short n_max_root_dir_entries_16;
    unsigned short n_logical_sectors_16;
    unsigned char media_descriptor;
    unsigned short logical_sector_per_fat_16;

    // DOS 3.31 BPB
    unsigned short physical_sector_per_track;
    unsigned short n_heads;
    unsigned int n_hidden_sectors;
    unsigned int n_sectors_32;

    // FAT32 ext BIOS param
    unsigned int n_sectors_per_fat_32;
    unsigned short mirror_flags;
    unsigned short version;
    unsigned int root_dir_start_cluster_num;
    unsigned short fs_info_sector_num;
    unsigned short boot_sector_bak_first_sector_num;
    unsigned int reserved[3];
    unsigned char physical_drive_num;
    unsigned char unused;
    unsigned char extended_boot_signature;
    unsigned int volume_id;
    unsigned char volume_label[11];
    unsigned char fay_system_type[8];
} __attribute__((packed));

struct fat32_metadata {
    unsigned int fat_region_blk_idx;
    unsigned int n_fat;
    unsigned int sector_per_fat;
    unsigned int data_region_blk_idx;
    unsigned int first_cluster;
    unsigned char sector_per_cluster;
};

struct fat32_internal {
    unsigned int first_cluster;
    unsigned int dirent_cluster;
    unsigned int size;
};

struct fat32_dirent {
    unsigned char name[8];
    unsigned char ext[3];
    unsigned char attr;
    unsigned char unknown;
    unsigned char create_time[3];
    unsigned short create_data;
    unsigned short last_access_data;
    unsigned short cluster_high;
    unsigned int ext_attr;
    unsigned short cluster_low;
    unsigned int size;
} __attribute__((packed));

int fat32_setup_mount(struct filesystem *fs, struct mount *mount);
struct dentry *fat32_create_dentry(struct dentry *parent, const char *name, enum node_type type);
struct vnode *fat32_create_vnode(struct dentry *dentry);

int fat32_register();

// f_ops
int fat32_read(struct file *file, void *buf, unsigned long len);
int fat32_write(struct file *file, const void *buf, unsigned long len);
// v_ops
int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_ls(struct vnode *vnode);
int fat32_load_dentry(struct dentry *dentry, const char *component_name);

extern struct fat32_metadata fat32_metadata;

#endif
