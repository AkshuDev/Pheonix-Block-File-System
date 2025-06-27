#include <pefi.h>
#include <pefi_types.h>
#include <pefi_priv.h>
#include <pefi_init.h>

LIB PEFI_InternalState pefi_state = {0};

EFI_BOOT_SERVICES *gBS = NULL;
EFI_GUID gEfiBlockIoProtocolGuid = { 0x964e5b21, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

LIB int InitalizeLib(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) {
    // Check if already initialized
    if (pefi_state.initialized) {
        return PEFI_ALREADY_INITIALIZED;
    }

    // Set system table
    pefi_state.system_table = SystemTable;

    // Set other stuff
    pefi_state.version = PEFI_VERSION;
    pefi_state.boot_services = SystemTable->BootServices;
    pefi_state.image_handle = ImageHandle;
    pefi_state.runtime_services = SystemTable->RuntimeServices;
    pefi_state.initialized = 1;

    gBS = SystemTable->BootServices;

    return 0;
}
