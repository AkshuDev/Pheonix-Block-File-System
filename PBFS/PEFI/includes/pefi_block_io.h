#pragma once
#include <pefi.h>
#include <pefi_media.h>

#define EFI_BLOCK_IO_PROTOCOL_GUID { \
    0x964e5b21, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
}

typedef struct EFI_BLOCK_IO_PROTOCOL EFI_BLOCK_IO_PROTOCOL;

typedef EFI_STATUS(EFIAPI * EFI_BLOCK_RESET) (IN EFI_BLOCK_IO_PROTOCOL *This, IN BOOLEAN ExtendedVerification);
typedef EFI_STATUS(EFIAPI * EFI_BLOCK_READ) (IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba, IN UINTN BufferSize, OUT VOID *Buffer);
typedef EFI_STATUS(EFIAPI * EFI_BLOCK_WRITE) (IN EFI_BLOCK_IO_PROTOCOL *This, IN UINT32 MediaId, IN EFI_LBA Lba, IN UINTN BufferSize, IN VOID *Buffer);
typedef EFI_STATUS(EFIAPI * EFI_BLOCK_FLUSH) (IN EFI_BLOCK_IO_PROTOCOL *This);

struct EFI_BLOCK_IO_PROTOCOL {
    UINT64 Revision;
    EFI_BLOCK_IO_MEDIA* Media;
    EFI_BLOCK_RESET Reset;
    EFI_BLOCK_READ ReadBlocks;
    EFI_BLOCK_WRITE WriteBlocks;
    EFI_BLOCK_FLUSH FlushBlocks;
};