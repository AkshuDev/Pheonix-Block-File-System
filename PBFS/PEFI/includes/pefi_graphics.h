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

struct EFI_PIXEL_BITMASK{
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t ReservedMask;
};

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION{
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    EFI_PIXEL_BITMASK PixelInformation;
    uint32_t PixelsPerScanLine;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE{
    uint32_t MaxMode;
    uint32_t Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    uint64_t SizeOfInfo;
    void* FrameBufferBase;
    uint64_t FrameBufferSize;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL{
    EFI_STATUS (EFIAPI *QueryMode)(void* This, uint32_t ModeNumber, uint64_t* SizeOfInfo, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION** Info);
    EFI_STATUS (EFIAPI *SetMode)(void* This, uint32_t ModeNumber);
    EFI_STATUS (EFIAPI *Blt)(void*, void*, int, int, int, int, int, int, int);
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
};

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
