#include <pefi.h>
#include <pefi_types.h>
#include <pefi_priv.h>
#define BUILD_LIB
#include <pefilib.h>

LIB PEFI_InternalState pefi_state = {0};

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

    return 0;
}
