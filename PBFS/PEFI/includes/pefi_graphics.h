#pragma once

// LIB
#define LIB EFIAPI

#include "pefi_types.h"
#include "pefi.h"

// Now for forward declaring structs
// efi_graphics
typedef struct EFI_PIXEL_BITMASK EFI_PIXEL_BITMASK;
typedef struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

#ifdef __cplusplus
extern "C" {
#endif

// efi_graphics
LIB extern EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
LIB extern EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
LIB EFI_STATUS pefi_init_gop(EFI_SYSTEM_TABLE* SystemTable) __attribute__((used));
LIB void pefi_draw_pixel(uint32_t x, uint32_t y, uint32_t color) __attribute__((used));
LIB void pefi_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) __attribute__((used));

#ifdef __cplusplus
}
#endif