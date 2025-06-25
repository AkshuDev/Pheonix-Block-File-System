#pragma once

#include <pefi.h>
#include <pefi_types.h>

// From the dll
#ifdef _WIN32
// DLL MODE
#ifdef BUILD_LIB
#define LIB __declspec__(dllexport)
#else
#define LIB __declspec__(dllimport)
#endif

// NOW we do stuff
// Define funcs
// First from efi_init
int LIB InitalizeLib(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle);
// main.c
void LIB efi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str);
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
// efi_memory
EFI_STATUS LIB EFIAPI Allocate(EFI_SYSTEM_TABLE *SystemTable, UINTN size, void** ptr);
EFI_STATUS LIB EFIAPI Free(EFI_SYSTEM_TABLE* SystemTable, void* ptr);
// efi_disk
void LIB read_file(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle);
// efi_graphics
EFI_STATUS LIB init_gop(EFI_SYSTEM_TABLE* SystemTable);
void LIB draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void LIB fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
#endif
