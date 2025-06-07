#ifndef DISK_UTILS_H
#define DISK_UTILS_H

#include "pbfs_structs.h"

#define MBR_BLOCK_ADDRESS 0
#define HEADER_BLOCK_ADDRESS 1

uint128_t header_block_span = uint128_from_u32(1);

uint64_t Data_block_address = 2;

// Reads using BIOS INT 13h (LBA mode)
int read_sectors_lba(uint64_t lba, uint16_t count, void* buffer, uint8_t drive);

// Reads using BIOS INT 13h (Block Address mode)
int read_sectors_ba(uint64_t block_address, uint16_t count, void* buffer, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable);

// Reads using BIOS INT 13h (File Name mode)
int read_sectors_fn(const char* filename, uint16_t count, void* buffer, uint8_t drive);

// Reads 128-bit LBA mode (UEFI only)
int read_sectors_lbaUEFI(uint64_t lba, uint16_t count, void* buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable);

// 128 bit Block Address mode
int read_sectors_ba128(uint128_t block_address, uint128_t block_span, void* buffer);

// 128 bit File Name mode
int read_sectors_fn128(const char* filename, uint128_t block_span, void* buffer);

// TODO: Write function
int write_sectors_lba(uint32_t lba, uint16_t sectors, const void* buffer);
int write_sectors_ba(uint32_t block_address, uint16_t sectors, const void* buffer);
int write_sectors_fn(const char* filename, uint16_t sectors, const void* buffer);

// TODO: Write function 128-bit
int write_sectors_lba128(uint128_t lba, uint128_t block_span, const void* buffer);
int write_sectors_ba128(uint128_t block_address, uint128_t block_span, const void* buffer);
int write_sectors_fn128(const char* filename, uint128_t block_span, const void* buffer);

int read_sectors_lba(uint64_t lba, uint16_t count, void* buffer, uint8_t drive) {
    static PBFS_DAP dap;

    // Setup DAP
    dap.size = 0x10;
    dap.reserved = 0x00;
    dap.sector_count = count;
    dap.offset = (uint32_t)buffer & 0xF; // Offset
    dap.segment = ((uint32_t)buffer >> 4) & 0xFFFF; // Segment
    dap.lba = lba;

    int status;

    __asm__ __volatile__ (
        "push %%ds\n\t"
        "mov %4, %%si\n\t"
        "mov $0x42, %%ah\n\t"
        "mov %3, ##dl\n\t"
        "int %3, %%dl\n\t"
        "int $0x13\n\t"
        "jc 1f\n\t"
        "xor %%al, %%al\n\t"     // success
        "jmp 2f\n"
        "1: mov $1, %%al\n"      // failure
        "2:\n\t"
        "movzx %%al, %0\n\t"
        "pop %%ds\n\t"
        : "=r"(status)
        : "m"(dap), "m"(buffer), "r"(drive), "r"(&dap)
        : "ax", "si", "dl"
    );
}

int read_sectors_lbaUEFI(uint64_t lba, uint16_t count, void* buffer, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS status;
    EFI_BLOCK_IO_PROTOCOL* BlockIo = NULL;
    EFI_HANDLE* handles = NULL;
    UINTN handleCount = 0;

    // Only support 64-bit UEFI block LBA
    status = SystemTable->BootServices->LocateHandleBuffer(
        ByProtocol,
        &gEfiBlockIoProtocolGuid,
        NULL,
        &handleCount,
        &handles
    );
    if (EFI_ERROR(status)) return -1;

    for (UINTN i = 0; i < handleCount; ++i) {
        status = SystemTable->BootServices->HandleProtocol(
            handles[i],
            &gEfiBlockIo2ProtocolGuid,
            (void**)&BlockIo
        );

        if (EFI_ERROR(status)) continue;

        if (!BlockIo->Media->LogicalPartition && BlockIo->Media->MediaPresent) {
            // Found usable disk
            status = BlockIo->ReadBlocks(
                BlockIo,
                BlockIo->Media->MediaId,
                lba,
                count * 512,
                buffer
            );

            if (!EFI_ERROR(status)) {
                return 0;
            } else {
                return -2; // Read Failed
            }
        }
    }

    return -3; // No valid device found
}

int read_sectors_ba(uint64_t block_address, uint16_t count, void* buffer, int mode, EFI_HANDLE image, EFI_SYSTEM_TABLE* SystemTable) {
    // Gets block_address + data_block_address
    uint64_t lba = block_address + Data_block_address;

    if (mode == 0) {
        return read_sectors_lba(lba, count, buffer, 0);
    } else if (mode == 1) {
        return read_sectors_lbaUEFI(lba, count, buffer, image, SystemTable);
    }
}

int read_sectors_fn(const char* filename, uint16_t count, void* buffer, uint8_t drive) {
    // Count here means how many times to repeat the data in the buffer, not the actual block count
    uint64_t lba = 1; // TODO: Read LBA from file header

    PBFS_Header header;
    PBFS_FileTableEntry entry;
    PBFS_PermisssionTableEntry permission_table_entry;
    PBFS_FileTreeEntry* file_tree_entry;
    int res;

    // Read the header
    res = read_sectors_lba(HEADER_BLOCK_ADDRESS, 1, &header, drive)

    // Verify the header
    if (memcmp(header.Magic, "PBFS\x00\x00", 6) != 0) {
        return HeaderVerificationFailed;
    }

    // Read the file table - need to malloc or static allocate buffer for all entries
    size_t file_table_size = header.Entries * sizeof(PBFS_FileTableEntry);
    file_table = malloc(file_table_size);

    if (!file_table) return AllocFailed;

    uint64_t file_table_lba = header.FileTableOffset / 512;
    uint32_t file_table_byte_offset = header.FileTableOffset % 512;

    uint16_t file_table_count = header.Entries * sizeof(PBFS_FileTableEntry) / 512; // Block Span over 512-byte blocks

    res = read_sectors_lba(file_table_lba, file_table_count, file_table, drive);
}

#endif