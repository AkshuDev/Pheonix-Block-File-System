#include "pefi_types.h"

#define BUILD_LIB
#include <pefilib.h>

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL{
    uint64_t Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(void* This, void** Root);
};

struct EFI_FILE_PROTOCOL{
    uint64_t Revision;
    EFI_STATUS (EFIAPI *Open)(void* This, void** NewHandle, uint16_t* FileName, uint64_t OpenMode, uint64_t Attributes);
    EFI_STATUS (EFIAPI *Read)(void* This, uint64_t* BufferSize, void* Buffer);
};

LIB EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = { 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

LIB EFI_STATUS read_file(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) {
    return 0; // Unimplemented
}
