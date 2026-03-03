#pragma once

#include <stddef.h>
#include <stdint.h>

#include <pbfs_structs.h>
#include <pbfs_structs_64.h>
#include <stdbool.h>

struct pbfs_funcs {
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
};

#ifdef PBFS_WDRIVERS

#else
struct block_device {
    const char *name;

    uint32_t block_size;
    uint64_t block_count;

    void *driver_data;

    int (*read_block)(
        struct block_device *dev,
        uint64_t lba,
        void *buffer
    );

    int (*write_block)(
        struct block_device *dev,
        uint64_t lba,
        const void *buffer
    );

    int (*flush)(
        struct block_device *dev
    );
};

struct pbfs_mount {
    bool active;

    struct block_device* dev;

    PBFS_Header header;
    PBFS_Header64 header64;

    PBFS_DMM root_dmm;
    PBFS_DMM64 root_dmm64;

    PBFS_Bitmap root_bitmap;
    PBFS_Bitmap64 root_bitmap64;

    PBFS_Kernel_Table root_kernel_table;
    PBFS_Kernel_Table64 root_kernel_table64;
    bool kernel_table_present;

    uint64_t partition_start_lba;
};

int pbfs_mount(struct block_device* dev, struct pbfs_mount* mnt) __attribute__((used));

int pbfs_read_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) __attribute__((used));
int pbfs_write_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) __attribute((used));
int pbfs_read(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) __attribute__((used));
int pbfs_write(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) __attribute((used));
int pbfs_flush(struct pbfs_mount* mnt) __attribute__((used));

int pbfs_add(struct pbfs_mount* mnt, char* path, uint32_t uid, uint32_t gid, PBFS_Metadata_Flags type, PBFS_Permission_Flags permissions, uint8_t* data, size_t data_size) __attribute__((used));
#endif

enum PBFS_Result {
    PBFS_RES_SUCCESS = 0,
    PBFS_ERR_UNKNOWN,

    PBFS_ERR_Device_Capacity_Too_Small,
    PBFS_ERR_Device_Block_Size_Too_Small,

    PBFS_ERR_No_Header_Present,
    PBFS_ERR_Invalid_Header,
    PBFS_ERR_Header_Unaligned,

    PBFS_ERR_Mount_Inactive,

    PBFS_ERR_No_Path,
    PBFS_ERR_Invalid_Path,

    PBFS_ERR_File_Not_Found,
    PBFS_ERR_No_Such_File_Or_Directory,
    PBFS_ERR_Invalid_File_Or_Directory,

    PBFS_ERR_Wrong_Type,
    PBFS_ERR_Wrong_Permissions,

    PBFS_ERR_Data_Unaligned,

    PBFS_ERR_No_Space_Left,
};