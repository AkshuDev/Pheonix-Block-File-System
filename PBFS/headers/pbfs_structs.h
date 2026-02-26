#pragma once

#include <linux/limits.h>
#include <stdint.h>

#define PBFS_MAGIC "PBFS\0\0"
#define PBFS_MAGIC_LEN 6
#define PBFS_BITMAP_LIMIT 496

#define PBFS_KERNEL_TABLE_ENTRIES 6
#define PBFS_DMM_ENTRIES 4

#define PBFS_MAX_BITMAP_CHAIN 50
#define PBFS_MAX_DMM_CHAIN 100
#define PBFS_MAX_KERNEL_CHAIN 2

typedef struct {
    uint32_t part[4]; // [0] = LSB, [3] = MSB
} uint128_t __attribute__((packed));

typedef struct {
    char magic[PBFS_MAGIC_LEN];
    uint128_t volume_id;

    uint64_t bitmap_lba; // <= 1 [Invalid/Not Present]
    uint64_t sysinfo_lba;
    uint64_t kernel_table_lba;
    uint64_t dmm_root_lba;
    uint64_t boot_partition_lba; // Size = (data_start_lba - boot_partition_lba) * block_size
    uint64_t data_start_lba;
    
    uint128_t total_blocks;
    uint32_t block_size;

    char disk_name[32];
    uint8_t reserved[6];
} PBFS_Header __attribute__((packed));

typedef struct {
    uint8_t bytes[PBFS_BITMAP_LIMIT];
    uint128_t extender_lba;
} PBFS_Bitmap __attribute__((packed));

typedef struct {
    char name[64];
    uint128_t lba;
} PBFS_Kernel_Entry __attribute__((packed));

typedef struct {
    PBFS_Kernel_Entry entries[PBFS_KERNEL_TABLE_ENTRIES];

    uint8_t reserved[16];
    uint128_t extender_lba;
} PBFS_Kernel_Table __attribute__((packed));

typedef struct {
    char name[64];
    uint128_t lba;
    uint32_t type;
    uint32_t perms;
    uint64_t created_timestamp;
    uint64_t modified_timestamp;
} PBFS_DMM_Entry __attribute__((packed));

typedef struct {
    PBFS_DMM_Entry entries[PBFS_DMM_ENTRIES];

    uint64_t entry_count;
    uint8_t reserved[88];
    uint128_t extender_lba;
} PBFS_DMM __attribute__((packed));

typedef struct {
    uint128_t ext_id;
    uint128_t next_lba;
} PBFS_Extender __attribute__((packed));

typedef enum {
    PERM_INVALID = 0,
    PERM_READ = 1 << 0,
    PERM_WRITE = 1 << 1,
    PERM_EXEC = 1 << 2,
    PERM_SYS = 1 << 3,
    PERM_PROTECTED = 1 << 4,
    PERM_LOCKED = 1 << 5 // No delete
} PBFS_Permission_Flags;

typedef enum {
    METADATA_FLAG_INVALID = 0,
    METADATA_FLAG_RES = 1 << 0,
    METADATA_FLAG_PBFS = 1 << 1,
    METADATA_FLAG_SYS = 1 << 2,
    METADATA_FLAG_FILE = 1 << 3,
    METADATA_FLAG_DIR = 1 << 4 
} PBFS_Metadata_Flags;

typedef struct {
    uint32_t uid;
    uint32_t gid;

    uint8_t permission_offset; // Cur LBA + Offset = PERM LBA (if 0, metadata ex_flags used as perm flags)

    uint64_t created_timestamp;
    uint64_t modified_timestamp;

    uint128_t data_size;
    uint32_t data_offset;

    uint32_t flags;
    uint32_t ex_flags;
    uint128_t extender_lba;
} PBFS_Metadata __attribute__((packed));

typedef struct {
    uint128_t hardware_bound_low;
    uint128_t hardware_bound_high;
    
    uint128_t access_log_lba;
    uint16_t acl_entries_count;

    uint8_t reserved[2];
} PBFS_Permission_Table __attribute__((packed));