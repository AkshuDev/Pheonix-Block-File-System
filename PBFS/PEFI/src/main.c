#include "pefi_types.h"
#define BUILD_LIB
#include <pefilib.h>

LIB void efi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

LIB EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    const uint16_t message[] = u"Pheonix EFI started!\r\n";
    efi_print(SystemTable, message);
    return 0;
}
