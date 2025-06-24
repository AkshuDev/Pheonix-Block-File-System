#pragma once

#include <stdint.h>
#include <stddef.h>

/* UEFI typedefs */
#include <pefi.h>

/* UEFI Table Header */
PEFI_PACKED_STRUCT(typedef struct {
    UINT64 Signature; // 8 bytes
    UINT32 Revision; // 4 bytes
    UINT32 HeaderSize; // 4 bytes
    UINT32 CRC32; // 4 bytes
    UINT32 Reserved; // 4 bytes
} EFI_TABLE_HEADER;) // 24 bytes

/* UEFI Runtime Services*/
#include <pefi_runtime.h>

/* Console Output Protocol */
#include <pefi_simple_text_out.h>

/* Boot Services */
#include <pefi_bootservices.h>
extern EFI_BOOT_SERVICES *gBS;
extern EFI_GUID gEfiBlockIoProtocolGuid;

/* Media */
#include <pefi_media.h>

/* Block IO */
#include <pefi_block_io.h>
#include <pefi_block_io2.h>

/* UEFI System Table */
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT;
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16* FirmwareVendor; // 8 bytes (pointer)
    UINT32 FirmwareRevision; // 4 bytes
    UINT32 __Pad1; // Padding
    EFI_HANDLE ConsoleInHandle; // 8 bytes
    EFI_SIMPLE_TEXT_INPUT* ConIn; // 8 bytes (pointer)
    EFI_HANDLE ConsoleOutHandle; // 8 bytes
    EFI_SIMPLE_TEXT_OUTPUT* ConOut; // 8 bytes
    EFI_HANDLE StdErrHandle; // 8 bytes
    EFI_SIMPLE_TEXT_OUTPUT* StdErr; // 8 bytes
    EFI_RUNTIME_SERVICES* RuntimeServices; // 8 bytes
    EFI_BOOT_SERVICES* BootServices; // 8 bytes
    UINTN NumberOfTableEntries; // 8 bytes
    EFI_CONFIGURATION_TABLE* ConfigurationTable; // 8 bytes
} EFI_SYSTEM_TABLE; // 128 bytes
