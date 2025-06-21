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

static int pbfs_init(uint8_t drive) {
    if (read_lba(layout.Header_Start, 1, &superblock, drive) != 0)
    {
        return DiskError;
    }

    if (memcmp(superblock.Magic, "PBFS\x00\x00", 6) != 0) {
        return DiskCorrupted;
    }

    return EXIT_SUCCESS;
}

// Public Interface

typedef struct {
    int (*init) (uint8_t drive);

    void (*init_efi) (EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable);
    void (*init_bitmap) (uint8_t* bitmap, uint64_t total_blocks);
    uint64_t (*calculate_bitmap_block_count)(uint64_t total_blocks);
    int (*is_real_mode)(void);
    int (*get_drive_params_real)(DriveParameters* info, uint8_t drive);
    EFI_STATUS (*get_drive_params_uefi)(DriveParameters *params, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
    uint64_t (*get_drive_size_bytes)(DriveParameters* info, uint8_t drive);
    int (*is_block_free)(uint64_t block_number, uint8_t drive);
    PBFS_Layout (*recalculate_layout)(uint32_t entry_count, uint32_t total_filetree_entries, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, uint8_t drive);
    int (*shift_file_data)(uint8_t drive, uint32_t old_data_start_lba, uint32_t new_data_start_lba, PBFS_Header *header);
    uint32_t (*find_free_lba_span)(int count, uint8_t drive, EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable);
    int (*mark_block_free)(uint64_t block_number, uint8_t drive);
    int (*mark_block_used)(uint64_t block_number, uint8_t drive);

    // 64 bit mode - Read
    int (*read_lba)(uint64_t lba, uint16_t count, void *buffer, uint8_t drive);
    int (*read_lba_UEFI)(uint64_t lba, uint16_t count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
    int (*read_ba)(uint64_t block_address, uint16_t count, void *buffer, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
    int (*read_fn)(const char *filename, uint16_t count_to_repeat_buffer, void *buffer, uint8_t drive, const char *caller);

    // 64 bit mode - Write
    int (*write_lba)(uint64_t lba, uint16_t count, const void *buffer, uint8_t drive);
    EFI_STATUS (*write_lba_UEFI)(EFI_LBA lba, UINTN count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
    int (*write_ba)(uint64_t block_addr, uint16_t count, const void *buffer, uint8_t drive);
    int (*write_fn)(const char *filename, void *data, PBFS_Permissions *permissions, char **permission_granted_files, int file_count, uint8_t drive);

} PBFS_Interface;

static PBFS_Interface driver = {
    .init = pbfs_init,
    .init_efi = PBFS_EFI_INIT,
    .init_bitmap = PBFS_BITMAP_INIT,
    .calculate_bitmap_block_count = PBFS_CALCULATE_BITMAP_BLOCK_COUNT,
    .is_real_mode = PBFS_IS_REAL_MODE,
    .get_drive_params_real = PBFS_GET_DRIVE_PARAMETERS_REAL,
    .get_drive_params_uefi = PBFS_GET_DRIVE_PARAMETERS_UEFI,
    .get_drive_size_bytes = PBFS_GET_DRIVE_SIZE_BYTES,
    .is_block_free = PBFS_IS_BLOCK_FREE,
    .recalculate_layout = PBFS_RELOAD,
    .shift_file_data = PBFS_SHIFT_DATA,
    .find_free_lba_span = PBFS_FIND_FREE_DATA_SLOTS,
    .mark_block_free = PBFS_MARK_BLOCK_FREE,
    .mark_block_used = PBFS_MARK_BLOCK_USED,

    // 64 bit mode - Read
    .read_lba = PBFS_READ_LBA_BIOS,
    .read_lba_UEFI = PBFS_READ_LBA_UEFI,
    .read_ba = PBFS_READ_BLOCK_ADDRESS,
    .read_fn = PBFS_READ,

    // 64 bit mode - Write
    .write_lba = PBFS_WRITE_LBA_BIOS,
    .write_lba_UEFI = PBFS_WRITE_LBA_UEFI,
    .write_ba = PBFS_WRITE_BLOCK_ADDRESS,
    .write_fn = PBFS_WRITE
};

PBFS_Interface* pbfs_get_interface() {
    return &driver;
}