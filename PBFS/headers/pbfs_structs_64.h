#pragma once

#include <stdint.h>
#include "pbfs_structs.h"

typedef struct {
    char magic[PBFS_MAGIC_LEN];
    uint64_t volume_id;

    uint64_t bitmap_lba; // <= 1 [Invalid/Not Present]
    uint64_t sysinfo_lba;
    uint64_t kernel_table_lba;
    uint64_t dmm_root_lba;
    uint64_t boot_partition_lba; // Size = (data_start_lba - boot_partition_lba) * block_size
    uint64_t data_start_lba;
    
    uint64_t total_blocks;
    uint32_t block_size;

    char disk_name[PBFS_DISK_NAME_LEN];
    uint8_t reserved[6];
} PBFS_Header64 __attribute__((packed));

typedef struct {
    uint8_t bytes[PBFS_BITMAP_LIMIT];
    uint64_t extender_lba;
} PBFS_Bitmap64 __attribute__((packed));

typedef struct {
    char name[64];
    uint64_t lba;
    uint64_t count;
} PBFS_Kernel_Entry64 __attribute__((packed));

typedef struct {
    PBFS_Kernel_Entry64 entries[PBFS_KERNEL_TABLE_ENTRIES];
    uint64_t entry_count;

    uint8_t reserved[16];
    uint64_t extender_lba;
} PBFS_Kernel_Table64 __attribute__((packed));

typedef struct {
    char name[PBFS_MAX_NAME_LEN];
    uint64_t lba;
    uint32_t type;
    uint32_t perms;
    uint64_t created_timestamp;
    uint64_t modified_timestamp;
} PBFS_DMM_Entry64 __attribute__((packed));

typedef struct {
    PBFS_DMM_Entry64 entries[PBFS_DMM_ENTRIES];

    uint64_t entry_count;
    uint8_t reserved[88];
    uint64_t extender_lba;
} PBFS_DMM64 __attribute__((packed));

typedef struct {
    uint64_t ext_id;
    uint64_t next_lba;
} PBFS_Extender64 __attribute__((packed));

typedef struct {
    char name[PBFS_MAX_NAME_LEN];
    
    uint32_t uid;
    uint32_t gid;

    uint8_t permission_offset; // Cur LBA + Offset = PERM LBA (if 0, metadata ex_flags used as perm flags)

    uint64_t created_timestamp;
    uint64_t modified_timestamp;

    uint64_t data_size;
    uint32_t data_offset;

    uint32_t flags;
    uint32_t ex_flags;
    uint64_t extender_lba;
} PBFS_Metadata64 __attribute__((packed));

typedef struct {
    uint64_t hardware_bound_low;
    uint64_t hardware_bound_high;
    
    uint64_t access_log_lba;
    uint16_t acl_entries_count;

    uint8_t reserved[2];
} PBFS_Permission_Table64 __attribute__((packed));