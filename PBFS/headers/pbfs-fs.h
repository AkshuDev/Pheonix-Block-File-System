#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pbfs_structs.h"
#include "pbfs_structs_64.h"
#include <stdbool.h>

#define PBFS_DEF_BLOCK_SIZE 512
#define PBFS_HDR_START_LBA 256
#define PBFS_HDR_START_BYTE (PBFS_DEF_BLOCK_SIZE * 256)

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

    int (*read)(
        struct block_device *dev,
        uint64_t lba,
        uint64_t count,
        void *buffer
    );

    int (*write)(
        struct block_device *dev,
        uint64_t lba,
        uint64_t count,
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


int pbfs_init(struct pbfs_funcs* functions) __attribute__((used));
int pbfs_format(struct block_device* dev, uint8_t reserve_kernel_table, uint8_t boot_part_size, uint64_t volume_id) __attribute__((used));
int pbfs_mount(struct block_device* dev, struct pbfs_mount* mnt) __attribute__((used));
int pbfs_read_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) __attribute__((used));
int pbfs_write_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) __attribute__((used));
int pbfs_read(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) __attribute__((used));
int pbfs_write(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) __attribute__((used));
int pbfs_flush(struct pbfs_mount* mnt) __attribute__((used));
int pbfs_add(struct pbfs_mount* mnt, char* path, uint32_t uid, uint32_t gid, PBFS_Metadata_Flags type, PBFS_Permission_Flags permissions, uint8_t* data, size_t data_size) __attribute__((used));
int pbfs_add_dir(struct pbfs_mount* mnt, char* path, uint32_t uid, uint32_t gid, PBFS_Permission_Flags permissions) __attribute__((used));
int pbfs_remove(struct pbfs_mount* mnt, char* path) __attribute__((used));
int pbfs_update_file(struct pbfs_mount* mnt, char* path, uint8_t* data, size_t data_size) __attribute__((used));
int pbfs_change_permissions(struct pbfs_mount* mnt, char* path, PBFS_Permission_Flags new_permissions) __attribute__((used));
int pbfs_read_file(struct pbfs_mount* mnt, char* path, uint8_t** data_out, size_t* data_size) __attribute__((used));
int pbfs_copy(struct pbfs_mount* mnt, char* path, char* new_path) __attribute__((used));
int pbfs_move(struct pbfs_mount* mnt, char* path, char* new_path) __attribute__((used));
int pbfs_rename(struct pbfs_mount* mnt, char* path, char* new_path) __attribute__((used));
int pbfs_find_entry(const char* path, PBFS_DMM_Entry* out, uint64_t* out_lba, struct pbfs_mount* mnt) __attribute__((used));
int pbfs_add_kernel(struct pbfs_mount* mnt, const char* name, uint8_t* data, size_t data_size) __attribute__((used));
int pbfs_find_kernel(struct pbfs_mount* mnt, const char* name, PBFS_Kernel_Entry* kernel_e) __attribute__((used));
int pbfs_get_kernel(struct pbfs_mount* mnt, PBFS_Kernel_Entry* kernel_e, uint8_t** data, size_t* data_size) __attribute__((used));
int pbfs_remove_kernel(struct pbfs_mount* mnt, char* name) __attribute__((used));
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
    PBFS_ERR_No_Data,

    PBFS_ERR_No_Space_Left,

    PBFS_ERR_Cannot_Remove_Root,

    PBFS_ERR_Allocation_Failed
};