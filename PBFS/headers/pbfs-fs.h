#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pbfs_structs.h"
#include "pbfs_structs_64.h"
#include <stdbool.h>

#define PBFS_DEF_BLOCK_SIZE 512
#define PBFS_HDR_START_LBA 512
#define PBFS_HDR_START_BYTE (PBFS_DEF_BLOCK_SIZE * PBFS_HDR_START_LBA)

#define PBFS_BOOT_PART_TYPE_NONE (1 << 0)
#define PBFS_BOOT_PART_TYPE_MBR (1 << 1)
#define PBFS_BOOT_PART_TYPE_GPT (1 << 3)

struct pbfs_funcs {
	bool initialized;
	
    void* (*malloc)(size_t size);
	void* (*realloc)(void* ptr, size_t nsize);
    void (*free)(void* ptr);
	uint64_t (*get_timestamp)(void);
};

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
	
	bool root_usage;
	uint32_t* root_uids;
	uint32_t* root_gids;

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

	PBFS_Bitmap64* bitmaps;
    uint64_t bitmap_count;
	uint64_t bitmap_cap;
};


int pbfs_init(struct pbfs_funcs* functions); // Initialize PBFS
int pbfs_format(struct block_device* dev, uint8_t reserve_kernel_table, uint64_t boot_part_lba, uint64_t boot_part_size, uint64_t volume_id); // Format Mounted File System
int pbfs_mount(struct block_device* dev, struct pbfs_mount* mnt); // Mount File System
int pbfs_enable_root(struct pbfs_mount* mnt, uint32_t* uid, uint32_t* gid); // Enables Root Usage in specified mount for to specific user/groups if present, else allows root on entire mount iself; this allows the bypass of permissions entirely (Multiple Calls allowed)
int pbfs_read_block(struct pbfs_mount* mnt, uint128_t fs_block, void* buffer); // Read a Block from Mounted File System
int pbfs_write_block(struct pbfs_mount* mnt, uint128_t fs_block, void* buffer); // Write a Block to Mounted File System
int pbfs_read(struct pbfs_mount* mnt, uint128_t fs_block, size_t size, void* buffer); // Read bytes from Mounted File System
int pbfs_write(struct pbfs_mount* mnt, uint128_t fs_block, size_t size, void* buffer); // Write bytes to Mounted File System
int pbfs_flush(struct pbfs_mount* mnt); // Flush Mounted File System
int pbfs_get_metadata(struct pbfs_mount* mnt, PBFS_DMM_Entry* entry, PBFS_Metadata* md_out, PBFS_Permission_Table* perm_out); // Retrieves the Metadata of the file/directory via DMM Entry
int pbfs_add(struct pbfs_mount* mnt, char* path, PBFS_Metadata* md, PBFS_Permission_Table* pt, uint8_t* data, size_t data_size); // Add a new file to Mounted File System
int pbfs_add_dir(struct pbfs_mount* mnt, char* path, PBFS_Metadata* md, PBFS_Permission_Table* pt); // Add a new directory to Mounted File System
int pbfs_remove(struct pbfs_mount* mnt, char* path); // Remove a directory/file from Mounted File System
int pbfs_update_file(struct pbfs_mount* mnt, char* path, uint8_t* data, size_t data_size); // Update a file from Mounted File System with new data
int pbfs_update_file_dmm(struct pbfs_mount* mnt, PBFS_DMM_Entry* entry, uint8_t* data, size_t data_size); // Update a file using DMM Entry for quick access, from Mounted File System with new data
int pbfs_change_permissions(struct pbfs_mount* mnt, char* path, PBFS_Permission_Flags new_permissions); // Change permissions of a directory/file from Mounted File System
int pbfs_change_permissions_dmm(struct pbfs_mount* mnt, PBFS_DMM_Entry* entry, PBFS_Permission_Flags new_permissions); // Change permissions of a directory/file using DMM Entry for quick access, from Mounted File System
int pbfs_read_file(struct pbfs_mount* mnt, char* path, uint8_t** data_out, size_t* data_size); // Read a file from Mounted File System
int pbfs_read_file_dmm(struct pbfs_mount* mnt, PBFS_DMM_Entry* entry, uint8_t** data_out, size_t* data_size); // Read a file using DMM Entry for quick access, from Mounted File System
int pbfs_copy(struct pbfs_mount* mnt, char* path, char* new_path); // Copy a diectory/file from Mounted File System to another place
int pbfs_move(struct pbfs_mount* mnt, char* path, char* new_path); // Move a directory/file from Mounted File System to another place
int pbfs_rename(struct pbfs_mount* mnt, char* path, char* new_path); // Rename a directory/file from Mounted File System
int pbfs_find_entry(struct pbfs_mount* mnt, const char* path, PBFS_DMM_Entry* out, uint64_t* out_lba); // Find a DMM entry based on path from Mounted File System
int pbfs_add_kernel(struct pbfs_mount* mnt, const char* name, PBFS_Kernel_Flags flags, uint8_t* data, size_t data_size); // Add a new kernel entry to Mounted File System
int pbfs_find_kernel(struct pbfs_mount* mnt, const char* name, PBFS_Kernel_Entry* kernel_e); // Find a Kernel Entry from Mounted File System
int pbfs_get_kernel(struct pbfs_mount* mnt, PBFS_Kernel_Entry* kernel_e, uint8_t** data, size_t* data_size); // Get Kernel data from Kernel Entry, from Mounted File System
int pbfs_remove_kernel(struct pbfs_mount* mnt, char* name); // Remove a Kernel Entry from Mounted File System
int pbfs_list_kernels(struct pbfs_mount* mnt, PBFS_Kernel_Entry* out, size_t max_out_len, size_t* out_len); // Get all Kernel Entries in Mounted File System
int pbfs_list_items(struct pbfs_mount* mnt, char* path, PBFS_DMM_Entry* out, size_t max_out_len, size_t* out_len); // List all items under a path in Mounted File System
int pbfs_add_bootloader(struct pbfs_mount* mnt, uint8_t* data, size_t data_size, uint8_t boot_part_type); // Add a bootloader to Mounted File System

enum PBFS_Result {
    PBFS_RES_SUCCESS = 0,
    PBFS_ERR_UNKNOWN,

    PBFS_ERR_Device_Capacity_Too_Small,
    PBFS_ERR_Device_Block_Size_Too_Small,
    
    PBFS_ERR_Bootloader_Partition_Too_Small,
    PBFS_ERR_No_Bootloader_Partition,

    PBFS_ERR_No_Header_Present,
    PBFS_ERR_Invalid_Header,
    PBFS_ERR_Header_Unaligned,

    PBFS_ERR_Mount_Inactive,
	PBFS_ERR_Not_Initialized,

    PBFS_ERR_No_Path,
    PBFS_ERR_Invalid_Path,

    PBFS_ERR_File_Not_Found,
    PBFS_ERR_No_Such_File_Or_Directory,
    PBFS_ERR_Invalid_File_Or_Directory,
    PBFS_ERR_File_Already_Exists,

    PBFS_ERR_Wrong_Type,
    PBFS_ERR_Wrong_Permissions,

    PBFS_ERR_Data_Unaligned,
    PBFS_ERR_No_Data,
    PBFS_ERR_Argument_Invalid,

    PBFS_ERR_No_Space_Left,

    PBFS_ERR_Cannot_Remove_Root,

    PBFS_ERR_Allocation_Failed,

    PBFS_ERR_Kernel_Table_Corrupted,
    PBFS_ERR_DMM_Corrupted,
    PBFS_ERR_Bitmap_Corrupted,
    PBFS_ERR_Sysinfo_Corrupted,
};

const char* pbfs_get_err_str(enum PBFS_Result return_code);