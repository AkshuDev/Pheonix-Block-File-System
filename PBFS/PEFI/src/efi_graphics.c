#include "pefi_types.h"

#define BUILD_LIB
#include <pefilib.h>

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

LIB EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
LIB EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

LIB EFI_STATUS init_gop(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_GUID gop_guid = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a} };
    return SystemTable->BootServices->LocateProtocol(&gop_guid, NULL, (void**)&GOP);
}

LIB void draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t* fb = (uint32_t*)GOP->Mode->FrameBufferBase;
    uint32_t width = GOP->Mode->Info->PixelsPerScanLine;
    fb[y * width + x] = color;
}

LIB void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            draw_pixel(x + i, y + j, color);
        }
    }
}
