#pragma once

// LIB
#define LIB

#include "pefi_types.h"
#include "pefi.h"

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL{
    uint64_t Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(void* This, void** Root);
};

struct EFI_FILE_PROTOCOL{
    uint64_t Revision;
    EFI_STATUS (EFIAPI *Open)(void* This, void** NewHandle, uint16_t* FileName, uint64_t OpenMode, uint64_t Attributes);
    EFI_STATUS (EFIAPI *Read)(void* This, uint64_t* BufferSize, void* Buffer);
};

EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = { 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

#ifdef __cplusplus
extern "C" {
#endif

// efi_disk
LIB EFI_BLOCK_IO_PROTOCOL* pefi_find_block_io(EFI_SYSTEM_TABLE* SystemTable) __attribute__((used));
LIB EFI_STATUS pefi_read_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo) __attribute__((used));
LIB EFI_STATUS pefi_write_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo) __attribute__((used));
LIB extern EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

#ifdef __cplusplus
}
#endif