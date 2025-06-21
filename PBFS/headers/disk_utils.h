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
int read_lbaUEFI(uint64_t lba, uint16_t count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable);
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

void efi_initialize(EFI_HANDLE Image_Handle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(Image_Handle, SystemTable);
}

void initialize_bitmap(uint8_t* bitmap, uint64_t total_blocks) {
    memset(bitmap, 0x00, BLOCK_SIZE); // All Free

    // Mark reserved blocks as used
    for (int i = 0; i < layout.Data_Start; i++) {
        uint64_t byte_index = i / 8;
        uint64_t bit_index = i % 8;
        bitmap[byte_index] |= (1 << bit_index);
    }
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
    uint64_t bitmap_block_index = block_number / BLOCKS_PER_BITMAP_BLOCK;
    uint64_t block_offset_in_bitmap = block_number % BLOCKS_PER_BITMAP_BLOCK;
    uint64_t byte_index = block_offset_in_bitmap / 8;
    uint8_t  bit_mask   = 1 << (block_offset_in_bitmap % 8);

    uint8_t bitmap_block[BLOCK_SIZE];
    uint64_t lba = layout.Bitmap_Start + bitmap_block_index;

    if (read_lba(lba, 1, bitmap_block, drive) != 0)
        return -1;

    return !(bitmap_block[byte_index] & bit_mask);
}

PBFS_Layout recalculate_layout(uint32_t entry_count, uint32_t total_filetree_entries, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, uint8_t drive)
{
    PBFS_Layout layout;

    layout.Header_Start = 1; // header is always 68 bytes → fits in 1 block and starts after mbr
    layout.Header_BlockSpan = 1;
    layout.Header_End = 2;

    // --- FILE TABLE ---
    uint32_t file_table_size = (entry_count + 1) * sizeof(PBFS_FileTableEntry);
    layout.FileTable_BlockSpan = (file_table_size + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;
    layout.FileTable_Start = 1 + layout.Header_BlockSpan;
    layout.FileTable_End = layout.FileTable_Start + layout.FileTable_BlockSpan;

    // --- PERMISSION TABLE ---
    uint32_t perm_table_size = (entry_count + 1) * sizeof(PBFS_PermissionTableEntry);
    layout.PermissionTable_BlockSpan = (perm_table_size + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;
    layout.PermissionTable_Start = layout.FileTable_Start + layout.FileTable_BlockSpan;
    layout.PermissionTable_End = layout.PermissionTable_Start + layout.PermissionTable_BlockSpan;

    // --- FILE TREE ---
    uint32_t file_tree_size = (total_filetree_entries + 1) * sizeof(PBFS_FileTreeEntry); // +1 for "~" entry
    layout.FileTree_BlockSpan = (file_tree_size + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;
    layout.FileTree_Start = layout.PermissionTable_Start + layout.PermissionTable_BlockSpan;
    layout.FileTree_End = layout.FileTree_Start + layout.FileTree_BlockSpan;

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
    layout.Bitmap_Start = layout.FileTree_Start + layout.FileTree_BlockSpan;
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
    uint64_t bitmap_block_index = block_number / BLOCKS_PER_BITMAP_BLOCK;
    uint64_t block_offset_in_bitmap = block_number % BLOCKS_PER_BITMAP_BLOCK;
    uint64_t byte_index = block_offset_in_bitmap / 8;
    uint8_t  bit_mask   = 1 << (block_offset_in_bitmap % 8);

    uint8_t bitmap_block[BLOCK_SIZE];
    uint64_t lba = layout.Bitmap_Start + bitmap_block_index;

    if (read_lba(lba, 1, bitmap_block, drive) != 0)
        return -1;

    bitmap_block[byte_index] |= bit_mask;

    return write_lba(lba, 1, bitmap_block, drive);
}

int mark_block_free(uint64_t block_number, uint8_t drive) {
    uint64_t bitmap_block_index = block_number / BLOCKS_PER_BITMAP_BLOCK;
    uint64_t block_offset_in_bitmap = block_number % BLOCKS_PER_BITMAP_BLOCK;
    uint64_t byte_index = block_offset_in_bitmap / 8;
    uint8_t  bit_mask   = 1 << (block_offset_in_bitmap % 8);

    uint8_t bitmap_block[BLOCK_SIZE];
    uint64_t lba = layout.Bitmap_Start + bitmap_block_index;

    if (read_lba(lba, 1, bitmap_block, drive) != 0)
        return -1;

    bitmap_block[byte_index] &= ~bit_mask;

    return write_lba(lba, 1, bitmap_block, drive);
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

int read_lbaUEFI(uint64_t lba, uint16_t count, void *buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;
    EFI_HANDLE *handles = NULL;
    UINTN handleCount = 0;

    // Only support 64-bit UEFI block LBA
    status = SystemTable->BootServices->LocateHandleBuffer(
        ByProtocol,
        &gEfiBlockIoProtocolGuid,
        NULL,
        &handleCount,
        &handles);
    if (EFI_ERROR(status))
        return -1;

    for (UINTN i = 0; i < handleCount; ++i)
    {
        status = SystemTable->BootServices->HandleProtocol(
            handles[i],
            &gEfiBlockIo2ProtocolGuid,
            (void **)&BlockIo);

        if (EFI_ERROR(status))
            continue;

        if (!BlockIo->Media->LogicalPartition && BlockIo->Media->MediaPresent)
        {
            // Found usable disk
            status = BlockIo->ReadBlocks(
                BlockIo,
                BlockIo->Media->MediaId,
                lba,
                count * 512,
                buffer);

            if (!EFI_ERROR(status))
            {
                return 0;
            }
            else
            {
                return -2; // Read Failed
            }
        }
    }

    return -3; // No valid device found
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
    // Count here means how many times to repeat the data in the buffer, not the actual block count
    uint64_t lba = 1; // TODO: Read LBA from file header

    PBFS_Header header;
    PBFS_FileTableEntry* entry = NULL;
    PBFS_PermissionTableEntry *permission_table_entry = NULL;
    PBFS_FileTreeEntry *file_tree_entry = NULL;
    int res;

    // Read the header
    res = read_lba(PBFS_HEADER_START_LBA, 1, &header, drive);

        if (res != 0)
    {
        return res;
    }

    // Verify the header
    if (memcmp(header.Magic, "PBFS\x00\x00", 6) != 0)
    {
        return HeaderVerificationFailed;
    }

    // Read the file table - need to malloc or static allocate buffer for all entries
    size_t file_table_size = header.Entries * sizeof(PBFS_FileTableEntry);
    uint8_t *file_table = malloc(file_table_size);

    if (!file_table)
    {
        return AllocFailed;
    }

    memset(file_table, 0, file_table_size);

    uint64_t file_table_lba = header.FileTableOffset / 512; // Block number

    uint16_t file_table_count = header.Entries; // Block Span over 512-byte blocks

    res = read_lba(file_table_lba, file_table_count, file_table, drive);

    if (res != 0)
    {
        free(file_table);
        return res;
    }

    // Loop through the file table to find the file entry
    int found = 1;

    for (int i = 0; i < header.Entries; i++)
    {
        entry = ((PBFS_FileTableEntry *)file_table[i]);

        if (strcmp(entry->Name, filename) == 0)
        {
            // Found the file entry
            // Break the loop
            found = 0;
            break;
        }
    }

    // Check if entry was not found
    if (found != 0)
    {
        free(file_table);
        return FileNotFound;
    }

    // Read the permission table
    uint64_t permission_table_lba = uint128_to_u64(entry->Permission_Table_Offset) / 512;
    uint64_t *block = (uint64_t *)malloc(BIOS_BLOCK_SIZE);

    if (!block)
    {
        return AllocFailed;
    }

    memset(block, 0, BIOS_BLOCK_SIZE);

    res = read_lba(permission_table_lba, 2, block, drive); // Load 1 block as the permission_table_offset is not relative to any lba.

    if (res != 0)
    {
        free(block);
        free(file_table);
        return res;
    }

    // TODO: Go to -> permission table offset - block offset in bytes
    uint64_t permission_entry_offset = uint128_to_u64(entry->Permission_Table_Offset) % BIOS_BLOCK_SIZE;

    permission_table_entry = (PBFS_PermissionTableEntry *)(block + permission_entry_offset);

    // Check if permission table entry is valid
    if (permission_table_entry->File_Tree_Offset == 0)
    {
        free(block);
        free(file_table);
        return PermissionDenied;
    }

    free(block);

    uint64_t *blocks = (uint64_t *)malloc(BIOS_BLOCK_SIZE * 5);

    // Check if Permission Table Entry doesn't have read permission
    if (permission_table_entry->Read == 0)
    {
        // Read the file tree (load blocks in a for loop (5 blocks per loop))

        // Load 5 blocks
        res = read_lba(permission_table_entry->File_Tree_Offset / 512, 5, blocks, drive);

        if (res != 0)
        {
            free(blocks);
            free(file_table);
            return res;
        }

        // TODO: Go to -> file tree offset - block offset in bytes
        uint64_t file_tree_entry_offset = ((uint64_t)permission_table_entry->File_Tree_Offset % BIOS_BLOCK_SIZE);

        file_tree_entry = (PBFS_FileTreeEntry *)(blocks + file_tree_entry_offset);

        // Check if file tree entry is valid
        if (file_tree_entry->Name[0] != '~')
        {
            free(blocks);
            free(file_table);
            return PermissionDenied;
        }

        found = 0; // Just Precautions :)
        int lp_started = 0;

        while (1)
        {
            // Loop till it hits Name[0] == ~ again or all blocks are checked
            if (file_tree_entry->Name[0] == '~')
            {
                if (lp_started == 0)
                {
                    lp_started = 1;
                    continue;
                }
                break;
            }

            if (strcmp(file_tree_entry->Name, caller) == 0)
            {
                found = 1;
                break;
            }
        }

        // Free
        free(blocks);

        // Check if found
        if (found == 0)
        {
            free(file_table);
            return FileNotFound;
        }
    }

    // Now save the file data offset
    uint64_t FileDataOffset = uint128_to_u64(entry->File_Data_Offset);
    uint64_t FileBlockSpan = uint128_to_u64(entry->Block_Span);

    free(file_table);

    // Go to the file offset
    uint64_t file_data_lba = FileDataOffset / 512;

    res = read_lba(file_data_lba, FileBlockSpan, buffer, drive);

    if (res != 0)
    {
        return res;
    }

    // Check if count_to_repeat_buffer is above 1 if then repeat the already file data and write it to buffer
    if (count_to_repeat_buffer > 1)
    {
        for (int i = 1; i < count_to_repeat_buffer; i++)
        {
            // Save the buffer contents into a temp buffer and append the temp buffer to the buffer
            uint8_t temp_buffer[BIOS_BLOCK_SIZE];
            memcpy(temp_buffer, buffer, BIOS_BLOCK_SIZE);
            memcpy(buffer + BIOS_BLOCK_SIZE * i, temp_buffer, BIOS_BLOCK_SIZE);
        }
    }

    return EXIT_SUCCESS;
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
    EFI_BLOCK_IO_PROTOCOL *block_io = NULL;
    EFI_HANDLE *handles;
    UINTN handle_count;

    // Locate all handles supporting Block IO Protocol
    status = uefi_call_wrapper(SystemTable->BootServices->LocateHandleBuffer, 5, ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &handle_count, &handles);

    if (EFI_ERROR(status))
        return status;

    // Make sure media is present and no read-only
    if (!block_io->Media->MediaPresent || block_io->Media->ReadOnly)
        return EFI_ACCESS_DENIED;

    // Calculate Size in bytes
    UINTN block_size = block_io->Media->BlockSize;
    UINTN buffer_size = block_count * block_size;

    // Write blocks
    status = uefi_call_wrapper(block_io->WriteBlocks, 5, block_io, block_io->Media->MediaId, lba, buffer_size, buffer);

    return status;
}

int write_ba(uint64_t block_addr, uint16_t count, const void *buffer, uint8_t drive)
{
    return write_lba(block_addr + Data_block_address, count, buffer, drive);
}

int write_fn(const char *filename, void *data, PBFS_Permissions *permissions, char **permission_granted_files, int file_count, uint8_t drive)
{
    PBFS_Header header;
    PBFS_FileTableEntry entry;
    PBFS_PermissionTableEntry perm_entry;
    PBFS_FileTreeEntry file_tree_entry;

    int res = read_lba(PBFS_HEADER_START_LBA, 1, &header, drive);
    if (res != 0)
        return res;
    if (strncmp(header.Magic, "PBFS\0\0", 6) != 0)
        return DiskCorrupted;

    // Check for existing file
    for (uint32_t i = 0; i < header.Entries; i++)
    {
        uint32_t file_table_lba = header.FileTableOffset + (i * sizeof(PBFS_FileTableEntry)) / BIOS_BLOCK_SIZE;
        res = read_lba(file_table_lba, 1, &entry, drive);
        if (res != 0)
            return res;

        if (strncmp(entry.Name, filename, sizeof(entry.Name)) == 0)
            return FileAlreadyExists;
    }

    // Calculate new layout with one more file entry and total tree entries (existing + new)
    uint32_t total_tree_entries = /* function to calculate total existing tree entries */ +file_count + 1; // +1 for "~" marker
    PBFS_Layout new_layout = recalculate_layout(header.Entries + 1, total_tree_entries, NULL, NULL, drive);

    // Extract old data start LBA from header (you must store it somewhere)
    uint32_t old_data_start_lba = layout.Data_Start;

    // Shift data if new data start LBA is after old data start LBA
    if (new_layout.Data_Start > old_data_start_lba)
    {
        res = shift_file_data(drive, old_data_start_lba, new_layout.Data_Start, &header);
        if (res != 0)
            return res;
    }

    // Update header with new layout offsets
    header.FileTableOffset = new_layout.FileTable_Start;

    // Write updated header (will be written again later after entry count increment)
    res = write_lba(PBFS_HEADER_START_LBA, 1, &header, drive);
    if (res != 0)
        return res;

    // Prepare new file table entry
    memset(&entry, 0, sizeof(entry));
    strncpy(entry.Name, filename, sizeof(entry.Name) - 1);

    // Permissions table offset in bytes relative to permission table start
    entry.Permission_Table_Offset = uint128_from_u32(header.Entries * sizeof(PBFS_PermissionTableEntry));

    // Setup permission table entry for new file
    memset(&perm_entry, 0, sizeof(perm_entry));
    perm_entry.Read = permissions->Read;
    perm_entry.Write = permissions->Write;
    perm_entry.Executable = permissions->Executable;
    perm_entry.Listable = permissions->Listable;
    perm_entry.Hidden = permissions->Hidden;
    perm_entry.Full_Control = permissions->Full_Control;
    perm_entry.Delete = permissions->Delete;
    perm_entry.Special_Access = permissions->Special_Access;

    // Write file tree entries buffer setup
    uint32_t total_tree_entries_local = file_count + 1; // +1 for "~" end marker
    uint32_t tree_total_size = total_tree_entries_local * sizeof(PBFS_FileTreeEntry);
    uint32_t tree_required_blocks = (tree_total_size + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;

    uint8_t file_tree_buffer[BIOS_BLOCK_SIZE];
    memset(file_tree_buffer, 0, BIOS_BLOCK_SIZE);

    PBFS_FileTreeEntry *tree_ptr = (PBFS_FileTreeEntry *)file_tree_buffer;
    int index = 0;

    // Write file tree entries in blocks
    for (uint32_t blk = 0; blk < tree_required_blocks; blk++)
    {
        memset(file_tree_buffer, 0, BIOS_BLOCK_SIZE);

        for (int i = 0; i < BIOS_BLOCK_SIZE / sizeof(PBFS_FileTreeEntry); i++)
        {
            if (index < file_count)
            {
                strncpy(tree_ptr[i].Name, permission_granted_files[index], sizeof(tree_ptr[i].Name) - 1);
            }
            else if (index == file_count)
            {
                strncpy(tree_ptr[i].Name, "~", 2); // end marker
            }
            else
            {
                break;
            }
            index++;
        }

        res = write_lba(new_layout.FileTree_Start + blk, 1, file_tree_buffer, drive);
        if (res != 0)
            return res;
    }

    perm_entry.File_Tree_Offset = new_layout.FileTree_Start * BIOS_BLOCK_SIZE;

    // Calculate file data size and required blocks
    uint32_t file_size_bytes = strlen((char *)data);
    uint32_t required_blocks = (file_size_bytes + BIOS_BLOCK_SIZE - 1) / BIOS_BLOCK_SIZE;

    // Find free LBA span for file data, but since we've shifted data, it should start at new_layout.DataStartLBA + sum of previous file blocks
    uint32_t data_start_lba = find_free_lba_span(required_blocks, drive, NULL, NULL);
    if (data_start_lba == INVALID_LBA)
        return NoMemoryAvailable;

    // Write file data blocks
    res = write_lba(data_start_lba, required_blocks, data, drive);
    if (res != 0)
        return res;

    // Update new file table entry with file data offset and block span
    entry.File_Data_Offset = uint128_from_u32(data_start_lba * BIOS_BLOCK_SIZE);
    entry.Block_Span = uint128_from_u32(required_blocks);

    // Write new file table entry at correct LBA
    uint32_t new_file_table_lba = header.FileTableOffset + ((header.Entries) * sizeof(PBFS_FileTableEntry)) / BIOS_BLOCK_SIZE;
    res = write_lba(new_file_table_lba, 1, &entry, drive);
    if (res != 0)
        return res;

    // Write new permission entry
    uint32_t new_permission_table_lba = layout.PermissionTable_Start + header.Entries * sizeof(PBFS_PermissionTableEntry);
    res = write_lba(new_permission_table_lba, 1, &perm_entry, drive);
    if (res != 0)
        return res;

    // Increment entries and write header again with updated count
    header.Entries++;
    res = write_lba(PBFS_HEADER_START_LBA, 1, &header, drive);
    if (res != 0)
        return res;

    return EXIT_SUCCESS;
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