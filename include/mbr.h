#ifndef MBR_H
#define MBR_H

struct mbr_partition {
    unsigned char status_flag;
    unsigned char partition_begin_head;
    unsigned short partition_begin_sector;
    unsigned char partition_type;
    unsigned char partition_end_head;
    unsigned short partition_end_sector;
    unsigned int starting_sector;
    unsigned number_of_sector;
};

#endif
