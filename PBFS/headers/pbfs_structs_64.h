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
    uint64_t boot_partition_size;
    uint64_t boot_partition_lba;
    uint64_t data_start_lba;
    
    uint64_t total_blocks;
    uint32_t block_size;

    char disk_name[PBFS_DISK_NAME_LEN];
} PBFS_Header64;

typedef struct {
	uint64_t next_lba;
	uint64_t prev_lba;
	uint64_t root_lba;
} PBFS_Extender64;

typedef struct {
    uint8_t bytes[PBFS_BITMAP_LIMIT];
    uint64_t extender_lba;
} PBFS_Bitmap64;

typedef struct {
    char name[32];
	PBFS_Kernel_Flags flags;
    uint64_t lba;
    uint64_t count;
} PBFS_Kernel_Entry64;

typedef struct {
    PBFS_Kernel_Entry64 entries[PBFS_KERNEL_TABLE_ENTRIES];
    uint8_t entry_count; // 255 Entries only

    PBFS_Extender64 extender;
} PBFS_Kernel_Table64;

typedef struct {
    uint128_t hash; // HASH is 128-bit hash only
	uint64_t lba;
} PBFS_DMM_Entry64;

typedef struct {
    PBFS_DMM_Entry64 entries[PBFS_DMM_ENTRIES];

    uint64_t entry_count;
    PBFS_Extender64 extender;
} PBFS_DMM64;

typedef struct {
    char name[PBFS_MAX_NAME_LEN];

	PBFS_Metadata_Types type;
    
    uint32_t uid;
    uint32_t gid;

    uint64_t permission_lba;

    uint64_t created_timestamp;
    uint64_t modified_timestamp;

    uint64_t data_size;
    uint64_t data_lba;

    uint32_t flags;
    uint32_t permission_flags;

    PBFS_Extender64 extender;
} PBFS_Metadata64;

typedef struct {
    uint64_t hardware_bound_low;
    uint64_t hardware_bound_high;
    
    uint64_t access_log_lba;
    uint32_t acl_entries_count;
} PBFS_Permission_Table64;