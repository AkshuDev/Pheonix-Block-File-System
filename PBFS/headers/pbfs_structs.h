#pragma once

#include <stdint.h>

#define PBFS_MAGIC "PBFS\0\0"
#define PBFS_MAGIC_LEN 6
#define PBFS_BITMAP_LIMIT 496
#define PBFS_DISK_NAME_LEN 32

#define PBFS_KERNEL_TABLE_ENTRIES 6
#define PBFS_DMM_ENTRIES 14

#define PBFS_MAX_DMM_CHAIN 2048
#define PBFS_MAX_KERNEL_CHAIN 2

#define PBFS_MAX_KERNEL_NAME_LEN 32
#define PBFS_MAX_NAME_LEN 64
#define PBFS_MAX_PATH_LEN 4096

typedef struct {
    uint32_t part[4]; // [0] = LSB, [3] = MSB
} uint128_t;

typedef struct {
    char magic[PBFS_MAGIC_LEN];
    uint128_t volume_id;

    uint64_t bitmap_lba; // <= 1 [Invalid/Not Present]
    uint64_t sysinfo_lba;
    uint64_t kernel_table_lba;
    uint64_t dmm_root_lba;
    uint64_t boot_partition_size;
    uint64_t boot_partition_lba;
    uint64_t data_start_lba;
    
    uint128_t total_blocks;
    uint32_t block_size;

    char disk_name[32];
} PBFS_Header;

typedef struct {
	uint128_t next_lba;
	uint128_t prev_lba;
	uint128_t root_lba;
} PBFS_Extender;

typedef struct {
    uint8_t bytes[PBFS_BITMAP_LIMIT];
    uint128_t extender_lba; // Once Created cannot be removed, and so 
} PBFS_Bitmap;

typedef enum {
    KERNEL_FLAG_CHAINLOADED = 1 << 0,
	KERNEL_FLAG_CONNECTOR = 1 << 1,
} PBFS_Kernel_Flags;

typedef struct {
    char name[PBFS_MAX_KERNEL_NAME_LEN];
	PBFS_Kernel_Flags flags;
    uint128_t lba;
    uint128_t count;
} PBFS_Kernel_Entry;

typedef struct {
    PBFS_Kernel_Entry entries[PBFS_KERNEL_TABLE_ENTRIES];
    uint8_t entry_count; // 255 Kernels only

    PBFS_Extender extender;
} PBFS_Kernel_Table;

typedef struct {
    uint128_t hash;
    uint128_t lba;
} PBFS_DMM_Entry;

typedef struct {
    PBFS_DMM_Entry entries[PBFS_DMM_ENTRIES];

    uint64_t entry_count;
    PBFS_Extender extender;
} PBFS_DMM;

typedef enum {
    PBFS_PERM_INVALID = 0,
    PBFS_PERM_READ = (1 << 0),
    PBFS_PERM_WRITE = (1 << 1),
    PBFS_PERM_EXEC = (1 << 2),
    PBFS_PERM_SYS = (1 << 3),
    PBFS_PERM_LOCKED = (1 << 4), // No delete
	PBFS_PERM_PROTECTED = (1 << 5) // No Delete, Read, Write, Execute, OR CHANGE OF PERMISSIONS
} PBFS_Permission_Flags;

typedef enum {
    PBFS_METADATA_TYPE_INVALID = 0,
    PBFS_METADATA_TYPE_RES = (1 << 0),
    PBFS_METADATA_TYPE_PBFS = (1 << 1),
    PBFS_METADATA_TYPE_SYS = (1 << 2),
    PBFS_METADATA_TYPE_FILE = (1 << 3),
    PBFS_METADATA_TYPE_DIR = (1 << 4),
	PBFS_METADATA_TYPE_SYMLINK = (1 << 5),
} PBFS_Metadata_Types;

typedef struct {
    char name[PBFS_MAX_NAME_LEN];

	PBFS_Metadata_Types type;
    
    uint32_t uid;
    uint32_t gid;

    uint128_t permission_lba;

    uint64_t created_timestamp;
    uint64_t modified_timestamp;

    uint128_t data_size;
    uint128_t data_lba;

    uint32_t flags;
    PBFS_Permission_Flags permission_flags;
	
    PBFS_Extender extender;
} PBFS_Metadata;

typedef struct {
    uint128_t hardware_bound_low;
    uint128_t hardware_bound_high;
    
    uint128_t access_log_lba;
    uint32_t acl_entries_count;
} PBFS_Permission_Table;