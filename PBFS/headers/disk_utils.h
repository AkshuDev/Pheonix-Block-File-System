#ifndef DISK_UTILS_H
#define DISK_UTILS_H

#include "pbfs.h"
#include "pbfs_structs.h"

#define MBR_BLOCK_ADDRESS 0
#define PBFS_HEADER_START_LBA 1
#define BIOS_BLOCK_SIZE 512
#define INVALID_LBA 0xFFFFFFFF // 4294967295

#define BLOCKS_PER_BITMAP_BLOCK 512

uint128_t header_block_span = (uint128_t){0, 0, 0, 1};

uint64_t Data_block_address = 2;

uint64_t Permission_Block_Count = 1;

PBFS_Layout layout;

PBFS_FileListEntry* file_list = NULL;     // pointer to array of PBFS_FileListEntry* (filenames)
size_t file_list_size = 0;   // number of valid entries
size_t file_list_capacity = 0;  // how many slots are allocated

// Asm imports
extern uint16_t CALLCONV get_drive_params_real_asm(uint16_t buffer_seg_offset, uint8_t drive);
extern int CALLCONV read_lba_asm(PBFS_DAP *dap, uint8_t drive);
extern int CALLCONV write_lba_asm(PBFS_DAP *dap, uint8_t drive);

// Forward Declared funcs for global use
// Tools
void efi_initialize(EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable);
void initialize_bitmap(uint8_t* bitmap, uint64_t total_blocks);
uint64_t calculate_bitmap_block_count(uint64_t total_blocks);
int is_real_mode();
int get_drive_params_real(DriveParameters* info, uint8_t drive);
EFI_STATUS get_drive_params_uefi(DriveParameters *params, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
uint64_t get_drive_size(DriveParameters* info, uint8_t drive);
int is_block_free(uint64_t block_number, uint8_t drive);
PBFS_Layout recalculate_layout(uint32_t entry_count, uint32_t total_filetree_entries, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, uint8_t drive);
int shift_file_data(uint8_t drive, uint32_t old_data_start_lba, uint32_t new_data_start_lba, PBFS_Header *header);
uint32_t find_free_lba_span(int count, uint8_t drive, EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable);
int mark_block_free(uint64_t block_number, uint8_t drive);
int mark_block_used(uint64_t block_number, uint8_t drive);

// 64 bit mode - Read
int read_lba(uint64_t lba, uint16_t count, void *buffer, uint8_t drive);
int read_lbaUEFI(EFI_LBA lba, UINTN count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
int read_ba(uint64_t block_address, uint16_t count, void *buffer, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
int read_fn(const char *filename, uint16_t count_to_repeat_buffer, void *buffer, uint8_t drive, const char *caller);

// 64 bit mode - Write
int write_lba(uint64_t lba, uint16_t count, const void *buffer, uint8_t drive);
EFI_STATUS write_lbaUEFI(EFI_LBA lba, UINTN count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
int write_ba(uint64_t block_addr, uint16_t count, const void *buffer, uint8_t drive);
int write_fn(const char *filename, void *data, PBFS_Permissions *permissions, char **permission_granted_files, int file_count, uint8_t drive);

// 128 bit mode - Read
int read_lba128(uint128_t block_address, uint128_t block_span, void *buffer, uint8_t drive);
int read_ba128(uint128_t block_address, uint128_t block_span, void *buffer);
int read_fn128(const char *filename, uint128_t block_span, void *buffer);

// 128 bit mode - Write
int write_lba128(uint128_t block_address, uint128_t block_span, const void *buffer, uint8_t drive);
int write_ba128(uint128_t block_address, uint128_t block_span, const void *buffer);
int write_fn128(const char *filename, uint128_t block_span, const void *buffer);

// Extracts filenames
int extract_filename(uint8_t* start, char* out_filename, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        if (start[i] == 0x00 && start[i+1] == 0x00) {
            break;
        }
        out_filename[i] = start[i];
        i++;
    }
    out_filename[i] = "\0\0";
}

// Checks for duplicate files
int check_duplicate(const char* filename) {
    for (size_t i = 0; i < file_list_size; i++) {
        if (strcmp(file_list[i].name, filename) == 0) return FileAlreadyExists;
    }
    return EXIT_SUCCESS;
}

// Adds a file to the file list in memory (RAM)
int add_file_to_list(const char* filename, uint32_t file_data_lba) {
    // Check for duplicates
    for (size_t i = 0; i < file_list_size; i++) {
        if (strcmp(file_list[i].name, filename) == 0) return FileAlreadyExists;
    }

    // Resize if needed
    if (file_list_size == file_list_capacity) {
        size_t new_capacity = (file_list_capacity == 0) ? 8 : file_list_capacity * 2;
        PBFS_FileListEntry* new_list = realloc(file_list, new_capacity * sizeof(PBFS_FileListEntry));
        if (!new_list) return AllocFailed;
        file_list = new_list;
        file_list_capacity = new_capacity;
    }

    // Add new entry
    file_list[file_list_size].name = strdup(filename);
    file_list[file_list_size].lba = file_data_lba;
    file_list_size++;

    return EXIT_SUCCESS;
}

// Frees file list
int free_file_list() {
    for (size_t i = 0; i < file_list_size; i++) {
        free(file_list[i].name);
    }
    free(file_list);
    file_list = NULL;
    file_list_size = 0;
    file_list_capacity = 0;
    return EXIT_SUCCESS;
}

// Finds a file in the file list
int find_file_in_file_list(const char* filename) {
    for (size_t i = 0; i < file_list_size; i++) {
        if (strcmp(file_list[i].name, filename) == 0) return i;
    }
    return FileNotFound;
}

void efi_initialize(EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(Image_Handle, SystemTable);
}

void initialize_bitmap(uint8_t* bitmap, uint64_t total_blocks) {

}

uint64_t calculate_bitmap_block_count(uint64_t total_blocks) {
    return (total_blocks + BLOCKS_PER_BITMAP_BLOCK - 1) / BLOCKS_PER_BITMAP_BLOCK;
}

int is_real_mode()
{
    uint16_t cs_val = 0;
    __asm__ __volatile__(
        "mov %%cs, %0"
        : "=r"(cs_val));

    return cs_val < 0x1000; // True for real else false for protected
}

int get_drive_params_real(DriveParameters* info, uint8_t drive)
{
    if (is_real_mode() == 0) return -100; // Protected mode - not supported in this func

    info->size = 0x1E;

    return get_drive_params_real_asm((uint16_t)(uintptr_t)info, drive);
}

uint64_t get_drive_total_blocks(uint64_t drive_size_in_bytes, uint32_t block_size)
{
    if (block_size == 0)
    {
        return 0;
    }
    return drive_size_in_bytes / block_size;
}

uint64_t get_drive_size(DriveParameters* disk_info, uint8_t drive)
{
    // INT 13h AH=48h
    // Setup registers:
    //  AH = 0x48
    //  DL = drive number (0x80 = first HDD)
    //  DS:SI = pointer to buffer (must be 30+ bytes)

    // BIOS will write to this buffer:

    int success = get_drive_params_real(disk_info, drive);

    if (!success)
        return 0;

    return disk_info->total_sectors * disk_info->bytes_per_sector;
}

int is_block_free(uint64_t block_number, uint8_t drive) {

}

PBFS_Layout recalculate_layout(uint32_t entry_count, uint32_t total_filetree_entries, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, uint8_t drive)
{
    PBFS_Layout layout;

    layout.Header_Start = 1; // header is always 68 bytes → fits in 1 block and starts after mbr
    layout.Header_BlockSpan = 1;
    layout.Header_End = 2;

    // --- BITMAP ---
    DriveParameters disk_info;
    if (is_real_mode() == 1){
        get_drive_params_real(&disk_info, drive);
    } else {
        // UEFI
        get_drive_params_uefi(&disk_info, ImageHandle, SystemTable);
    }
    uint32_t bitmap_size = get_drive_total_blocks(disk_info.bytes_per_sector * disk_info.total_sectors, BIOS_BLOCK_SIZE);
    layout.Bitmap_BlockSpan = (bitmap_size + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;
    layout.Bitmap_Start = layout.Header_End;
    layout.Bitmap_End = layout.Bitmap_Start + layout.Bitmap_BlockSpan;

    // --- DATA REGION ---
    layout.Data_Start = layout.Bitmap_End;

    return layout;
}

int shift_file_data(uint8_t drive, uint32_t old_data_start_lba, uint32_t new_data_start_lba, PBFS_Header *header)
{
    PBFS_FileTableEntry entry;
    int res;

    for (uint32_t i = 0; i < header->Entries; i++)
    {
        // Calculate LBA and read file table entry
        uint32_t file_table_lba = header->FileTableOffset + (i * sizeof(PBFS_FileTableEntry)) / BIOS_BLOCK_SIZE;
        res = read_lba(file_table_lba, 1, &entry, drive);
        if (res != 0)
            return res;

        // Calculate file's data start LBA and block span
        uint32_t old_file_data_lba = (uint32_t)(uint128_to_u64(entry.File_Data_Offset) / BIOS_BLOCK_SIZE);
        uint32_t blocks = uint128_to_u32(entry.Block_Span);

        if (blocks == 0)
            continue; // No data to move

        // Calculate new file data start LBA after shift
        uint32_t new_file_data_lba = old_file_data_lba + (new_data_start_lba - old_data_start_lba);

        // Allocate buffer (assume max 512 * N blocks, split if needed)
        uint8_t *buffer = malloc(blocks * BIOS_BLOCK_SIZE);
        if (!buffer)
            return -1; // malloc fail

        // Read old data blocks
        res = read_lba(old_file_data_lba, blocks, buffer, drive);
        if (res != 0)
        {
            free(buffer);
            return res;
        }

        // Write data to new location
        res = write_lba(new_file_data_lba, blocks, buffer, drive);
        if (res != 0)
        {
            free(buffer);
            return res;
        }

        free(buffer);

        // Update file table entry offset to new location
        entry.File_Data_Offset = uint128_from_u64((uint64_t)(new_file_data_lba * BIOS_BLOCK_SIZE));

        // Write updated file table entry back
        res = write_lba(file_table_lba, 1, &entry, drive);
        if (res != 0)
            return res;
    }
    return 0;
}

uint32_t find_free_lba_span(int count, uint8_t drive, EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable)
{
    DriveParameters drive_params;

    // Get drive parameters
    if (is_real_mode() == 1) {
        get_drive_params_real(&drive_params, drive);
    } else {
        get_drive_params_uefi(&drive_params, Image_Handle, SystemTable);
    }

    uint32_t max_blocks = drive_params.total_sectors * drive_params.bytes_per_sector / BIOS_BLOCK_SIZE;

    int consecutive = 0;
    uint32_t start_lba = 0;

    for (uint32_t lba = 0; lba < max_blocks; lba++)
    {
        if (is_block_free(lba, drive))
        {
            if (consecutive == 0)
                start_lba = lba;
            consecutive++;

            if (consecutive == count)
            {
                return start_lba; // found enough continuous free blocks
            }
        }
        else
        {
            consecutive = 0; // reset if block is used
        }
    }

    return INVALID_LBA; // no continuous span found
}

int mark_block_used(uint64_t block_number, uint8_t drive) {

}

int mark_block_free(uint64_t block_number, uint8_t drive) {

}


int read_lba(uint64_t lba, uint16_t count, void *buffer, uint8_t drive)
{
    PBFS_DAP dap = {
        .size = 0x10,
        .reserved = 0,
        .sector_count = count,
        .offset = ((uintptr_t)buffer) & 0xF,
        .segment = ((uintptr_t)buffer) >> 4,
        .lba = lba
    };
    return read_lba_asm(&dap, drive) == 0;
}

int read_lbaUEFI(EFI_LBA lba, UINTN count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_BLOCK_IO_PROTOCOL *BlockIo;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    UINTN Index;
    // Locate the Block I/O protocol
    status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(status)) {
        return -1; // Failed to locate Block I/O protocol
    }

    // Iterate through the handles to find a suitable Block I/O device
    for (Index = 0; Index < HandleCount; Index++) {
        status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
        if (EFI_ERROR(status)) {
            continue; // Skip this handle if it doesn't support Block I/O
        }
        // Check if the device is readable
        if (BlockIo->Media->MediaId == 0 && BlockIo->Media->ReadOnly == FALSE) {
            // Read from the specified LBA
            status = BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId, lba, count * BlockIo->Media->BlockSize, buffer);
            if (!EFI_ERROR(status)) {
                break; // Successfully read from the device
            }
        }
    }
    // Free the handle buffer
    if (HandleBuffer != NULL) {
        gBS->FreePool(HandleBuffer);
    }
    return EFI_ERROR(status) ? -1 : 0; // Return 0 on success, -1 on failure
}

int read_ba(uint64_t block_address, uint16_t count, void *buffer, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable)
{
    // Gets block_address + data_block_address
    uint64_t lba = block_address + Data_block_address;

    if (mode == 0)
    {
        return read_lba(lba, count, buffer, 0);
    }
    else if (mode == 1)
    {
        return read_lbaUEFI(lba, count, buffer, image, SystemTable);
    }
}

int read_fn(const char *filename, uint16_t count_to_repeat_buffer, void *buffer, uint8_t drive, const char *caller)
{
    // Check for file in the file list
    int file_index = find_file_in_file_list(filename);

    if (file_index == FileNotFound)
    {
        return file_index;
    }

    PBFS_FileListEntry file_entry = file_list[file_index];

    // Read the file data
    // Go to the file lba and read it
    uint64_t lba = file_entry.lba;

    uint16_t* metadata_buffer = malloc(block_size);

    read_lba(lba, 1, metadata_buffer, drive);

    // Now extract the structs

    PBFS_FileTableEntry* file_table_entry = (PBFS_FileTableEntry*)metadata_buffer;
    PBFS_PermissionTableEntry* permission_table_entry = (PBFS_PermissionTableEntry*)(sizeof(PBFS_FileTableEntry) + sizeof(PBFS_FileTableEntry));

    // Now first step is to check the permissions
    if (permission_table_entry->Read != 1) {
        // Check the file tree
        // Check if the caller has permission to read the file
        PBFS_FileTreeEntry* file_tree_entry = (PBFS_FileTreeEntry*)(sizeof(PBFS_FileTableEntry) + sizeof(PBFS_PermissionTableEntry));

        while (1) {

        }
    }
        return PermissionDenied;
    }


}


int write_lba(uint64_t lba, uint16_t count, const void *buffer, uint8_t drive)
{
    PBFS_DAP dap = {
        .size = 0x10,
        .reserved = 0,
        .sector_count = count,
        .offset = ((uintptr_t)buffer) & 0xF,
        .segment = ((uintptr_t)buffer) >> 4,
        .lba = lba
    };
    return write_lba_asm(&dap, drive) == 0;
}

EFI_STATUS write_lbaUEFI(EFI_LBA lba, UINTN block_count, void *buffer, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_BLOCK_IO_PROTOCOL *BlockIo;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    UINTN Index;

    // Locate the BlockIo protocol
    status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(status)) {
        return status;
    }

    // Iterate through handles to find a valid BlockIo protocol
    for (Index = 0; Index < HandleCount; Index++) {
        status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
        if (EFI_ERROR(status)) {
            continue; // Skip this handle
        }

        // Check if the device is removable or writable
        if (BlockIo->Media->MediaId == 0 && BlockIo->Media->ReadOnly == FALSE) {
            // Write to the specified LBA
            status = BlockIo->WriteBlocks(BlockIo, BlockIo->Media->MediaId, lba, block_count * BlockIo->Media->BlockSize, buffer);
            if (!EFI_ERROR(status)) {
                break;
            }
        }
    }

    // Free the handle buffer
    if (HandleBuffer != NULL) {
        gBS->FreePool(HandleBuffer);
    }

    return status;
}

int write_ba(uint64_t block_addr, uint16_t count, const void *buffer, uint8_t drive)
{
    return write_lba(block_addr + Data_block_address, count, buffer, drive);
}

int write_fn(const char *filename, void *data, PBFS_Permissions *permissions, char **permission_granted_files, int file_count, uint8_t drive)
{

}

// Tools
#define PBFS_EFI_INIT efi_initialize
#define PBFS_BITMAP_INIT initialize_bitmap
#define PBFS_CALCULATE_BITMAP_BLOCK_COUNT calculate_bitmap_block_count
#define PBFS_IS_REAL_MODE is_real_mode
#define PBFS_GET_DRIVE_PARAMETERS_REAL get_drive_params_real
#define PBFS_GET_DRIVE_PARAMETERS_UEFI get_drive_params_uefi
#define PBFS_GET_DRIVE_TOTAL_BLOCKS get_drive_total_blocks
#define PBFS_GET_DRIVE_SIZE_BYTES get_drive_size
#define PBFS_RELOAD recalculate_layout
#define PBFS_SHIFT_DATA shift_file_data
#define PBFS_FIND_FREE_DATA_SLOTS find_free_lba_span
#define PBFS_MARK_BLOCK_USED mark_block_used
#define PBFS_MARK_BLOCK_FREE mark_block_free
#define PBFS_IS_BLOCK_FREE is_block_free


// Read
#define PBFS_READ_LBA_BIOS read_lba
#define PBFS_READ_LBA_UEFI read_lbaUEFI
#define PBFS_READ_BLOCK_ADDRESS read_ba
#define PBFS_READ read_fn

// Write
#define PBFS_WRITE_LBA_BIOS write_lba
#define PBFS_WRITE_LBA_UEFI write_lbaUEFI
#define PBFS_WRITE_BLOCK_ADDRESS write_ba
#define PBFS_WRITE write_fn

#endif