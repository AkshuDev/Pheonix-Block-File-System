#pragma once

// LIB
#define LIB

#include "pefi_types.h"
#include "pefi.h"

// Now for forward declaring structs
// efi_disk
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