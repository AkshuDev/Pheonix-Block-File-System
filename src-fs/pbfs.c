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

// Public Interface

typedef struct {
    int (*init)(void);
    int (*read_file_bios)(uint64_t block_addr, uint16_t count, void* buffer, uint8_t drive);
    int (*read_file_uefi)(uint64_t block_addr, uint16_t count, void* buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable);
    int (*read_file_block_address)(uint64_t block_addr, uint16_t count, void* buffer, uint8_t drive, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable);
    int (*read_file)(const char* filename, uint16_t count_to_repeat, void* buffer, uint8_t drive);
    int (*write_file_bios)(uint64_t block_addr, uint16_t count, const void* buffer, uint8_t drive);
    EFI_STATUS (*write_file_uefi)(EFI_LBA lba, UINTN block_count, void* buffer, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
    int (*write_file_block_address)(uint64_t block_addr, uint16_t count, const void* buffer);
    int (*write_file)(const char* filename, void *data, PBFS_Permissions *permissions, char **permission_granted_files, int file_count, uint8_t drive);
    int (*efi_init)(EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable);
    PBFS_Layout (*reload_layout)(uint32_t entry_count, uint32_t total_filetree_entries);
    int (*shift_file_data)(uint8_t drive, uint32_t old_data_start_lba, uint32_t new_data_start_lba, PBFS_Header *header);
    uint32_t (*find_free_data_slot)(int count, uint8_t drive);

} PBFS_Interface;

static PBFS_Interface driver = {
    .init = pbfs_init,
    .read_file_bios = PBFS_READ_LBA_BIOS,
    .read_file_uefi = PBFS_READ_LBA_UEFI,
    .read_file_block_address = PBFS_READ_BLOCK_ADDRESS,
    .read_file = PBFS_READ,
    .write_file_bios = PBFS_WRITE_LBA_BIOS,
    .write_file_uefi = PBFS_WRITE_LBA_UEFI,
    .write_file_block_address = PBFS_WRITE_BLOCK_ADDRESS,
    .write_file = PBFS_WRITE,
    .efi_init = PBFS_EFI_INIT,
    .reload_layout = PBFS_RELOAD,
    .shift_file_data = PBFS_SHIFT_DATA,
    .find_free_data_slot = PBFS_FIND_FREE_DATA_SLOTS
};

PBFS_Interface* pbfs_get_interface() {
    return &driver;
}