#include <pefi_types.h>
#include <pefi.h>
#include <pefi_main.h>

LIB void pefi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, str);
}

LIB void pefi_clear(EFI_SYSTEM_TABLE* SystemTable) {
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
}

LIB void pefi_enable_cursor(EFI_SYSTEM_TABLE* SystemTable) {
    SystemTable->ConOut->EnableCursor(SystemTable->ConOut, TRUE);
}

LIB void pefi_disable_cursor(EFI_SYSTEM_TABLE* SystemTable) {
    SystemTable->ConOut->EnableCursor(SystemTable->ConOut, FALSE);
}

LIB EFI_STATUS pefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    const uint16_t message[] = u"Pheonix EFI started!\r\n";
    pefi_print(SystemTable, message);
    return 0;
}
