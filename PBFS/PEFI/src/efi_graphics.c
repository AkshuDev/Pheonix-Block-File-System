#include <pefi_types.h>
#include <pefi_graphics.h>

LIB EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
LIB EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

LIB EFI_STATUS pefi_init_gop(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_GUID gop_guid = { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a} };
    return SystemTable->BootServices->LocateProtocol(&gop_guid, NULL, (void**)&GOP);
}

LIB void pefi_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t* fb = (uint32_t*)GOP->Mode->FrameBufferBase;
    uint32_t width = GOP->Mode->Info->PixelsPerScanLine;
    fb[y * width + x] = color;
}

LIB void pefi_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            pefi_draw_pixel(x + i, y + j, color);
        }
    }
}
