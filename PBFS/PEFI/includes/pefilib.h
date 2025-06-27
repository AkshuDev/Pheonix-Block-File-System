#pragma once

#include <pefi.h>
#include <pefi_types.h>
#include <pefi_priv.h>

// From the dll
#ifdef _WIN32
// DLL MODE
#ifdef BUILD_LIB
#define LIB __declspec(dllexport)
#else
#define LIB __declspec(dllimport)
#endif
#else
// .SO MODE
#ifdef BUILD_LIB
#define LIB __attribute__((visibility("default")))
#else
#define LIB
#endif
#endif

// Now for forward declaring structs
// efi_disk
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct EFI_FILE_PROTOCOL;
// efi_graphics
typedef struct EFI_PIXEL_BITMASK EFI_PIXEL_BITMASK;
typedef struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

// NOW we do stuff
// Define funcs
#ifdef __cplusplus
extern "C" {
#endif
// First from efi_init
LIB int InitalizeLib(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle);
LIB extern PEFI_InternalState pefi_state;
// main.c
LIB void pefi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str);
LIB EFI_STATUS pefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
// efi_memory
LIB EFI_STATUS EFIAPI pefi_allocate(EFI_SYSTEM_TABLE *SystemTable, UINTN size, void** ptr);
LIB EFI_STATUS EFIAPI pefi_free(EFI_SYSTEM_TABLE* SystemTable, void* ptr);
// efi_disk
LIB EFI_BLOCK_IO_PROTOCOL* pefi_find_block_io(EFI_SYSTEM_TABLE* SystemTable);
LIB EFI_STATUS pefi_read_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo);
LIB EFI_STATUS pefi_write_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo);
LIB extern EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
// efi_graphics
LIB extern EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
LIB extern EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
LIB EFI_STATUS pefi_init_gop(EFI_SYSTEM_TABLE* SystemTable);
LIB void pefi_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
LIB void pefi_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
#ifdef __cplusplus
}
#endif
