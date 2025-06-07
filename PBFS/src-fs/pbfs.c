#include "pbfs.h"
#include "disk_utils.h"

// Internal state
static PBFS_Header superblock;
static PBFS_FileTableEntry file_table[MAX_FILES];

// Low-Level Helpers
uint128_t combine_offset_to_block(uint32_t offsetp1, uint32_t offsetp2, uint32_t offsetp3, uint32_t offsetp4) {
    return (uint128_t){offsetp1, offsetp2, offsetp3, offsetp4};
}

uint128_t combine_span_to_blocks(uint32_t spanp1, uint32_t spanp2, uint32_t spanp3, uint32_t spanp4) {
    return (uint128_t){spanp1, spanp2, spanp3, spanp4};
}

// PBFS Core

static int pbfs_init() {
    if (read_sectors_lba(0, 1, &superblock) != 0)
    {
        return DiskError;
    }

    if (memcmp(superblock.Magic, "PBFS\x00\x00", 6) != 0) {
        return DiskCorrupted;
    }

    uint32_t entries_size = superblock.Entries * sizeof(PBFS_FileTableEntry);
    uint32_t blocks_needed = (entries_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (read_sectors_lba(superblock.FileTableOffset, blocks_needed, file_table) != 0) {
        return Failure;
    }

    return EXIT_SUCCESS;
}

static pbfs_find(const char name[128], PBFS_FileTableEntry* out_entry) {
    uint128_t Entries128 = uint128_from_u32(superblock.Entries);
    uint128_t i = UINT128_ZERO;
    for (; uint128_cmp(&i, &Entries128) < 0; uint128_inc(&i))
    {
        uint32_t idx = i.part[0];
        if (strncmp(file_table[idx].Name, name, sizeof(file_table[idx].Name)) == 0) {
            *out_entry = file_table[idx];
            return EXIT_SUCCESS;
        }
    }

    return Failure;
}

static int pbfs_read_file(const char name[128], void* out_buffer, int mode) {
    PBFS_FileTableEntry entry;
    uint128_t file_data_offset = entry.File_Data_Offset;
    uint128_t block_span = entry.Block_Span;

    if (pbfs_find(name, &entry) != EXIT_SUCCESS) {
        return Failure;
    }

    // Read the file data corrrosponding with the offset
    uint128_add(&file_data_offset, &file_data_offset, &(uint128_t){sizeof(PBFS_Header), 0, 0, 0});

    // Now read the file data from file_data_offset to file_data_offset + block_span * block_size
    if (mode == 0)
    {
        // Lba mode
        return read_sectors_lba128(file_data_offset, block_span, out_buffer);
    } else if (mode == 1)
    {
        // Block Address mode
        return read_sectors_ba128(file_data_offset, block_span, out_buffer);
    } else if (mode == 2)
    {
        // File Name mode
        return read_sectors_fn128(file_data_offset, block_span, out_buffer);
    }
}

// Public Interface

typedef struct {
    int (*init)(void);
    int (*read_file)(const char* name, void* out_buffer);
    int (*find_file)(const char* name, PBFS_FileTableEntry* out_entry);
} PBFS_Interface;

static PBFS_Interface driver = {
    .init = pbfs_init,
    .read_file = pbfs_read_file,
    .find_file = pbfs_find
};

PBFS_Interface* pbfs_get_interface() {
    return &driver;
}