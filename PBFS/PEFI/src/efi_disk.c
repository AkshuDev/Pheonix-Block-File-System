#include "pefi_types.h"

#define BUILD_LIB
#include <pefilib.h>

LIB typedef struct {
    uint64_t Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(void* This, void** Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

LIB typedef struct {
    uint64_t Revision;
    EFI_STATUS (EFIAPI *Open)(void* This, void** NewHandle, uint16_t* FileName, uint64_t OpenMode, uint64_t Attributes);
    EFI_STATUS (EFIAPI *Read)(void* This, uint64_t* BufferSize, void* Buffer);
    // You can add Write, SetInfo, etc.
} EFI_FILE_PROTOCOL;

LIB EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = { 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

void LIB read_file(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) {
    void* SFSP;
    EFI_STATUS status = SystemTable->BootServices->LocateProtocol(&EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, NULL, &SFSP);

    if (status) return;

    void* Root;
    ((EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*)SFSP)->OpenVolume(SFSP, &Root);

    void* FileHandle;
    uint16_t FileName[] = u"test.txt";
    ((EFI_FILE_PROTOCOL*)Root)->Open(Root, &FileHandle, FileName, 1 /* Read Mode */, 0);

    char buffer[128];
    uint64_t size = sizeof(buffer);
    ((EFI_FILE_PROTOCOL*)FileHandle)->Read(FileHandle, &size, buffer);

    // Now buffer has the data.
}
