#include "pefi_types.h"
#include <pefi_main.h>

LIB void pefi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

LIB EFI_STATUS pefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    const uint16_t message[] = u"Pheonix EFI started!\r\n";
    pefi_print(SystemTable, message);
    return 0;
}
