#include <../PEFI/includes/pefilib.h>
#include <../PEFI/includes/pefi_main.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Clear screen to prove we are in control
    SystemTable->ConOut->Reset(SystemTable->ConOut, FALSE);
    pefi_print(SystemTable, u"PBFS UEFI Test: SUCESS\r\n");
    for (;;) {__asm__ volatile("hlt");}
}