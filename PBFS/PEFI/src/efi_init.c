#include <pefi.h>
#include <pefi_types.h>
#include <pefi_priv.h>
#include <pefi_init.h>
#include <pefi_variables.h>

LIB PEFI_InternalState pefi_state = {0};

EFI_BOOT_SERVICES *gBS = NULL;
EFI_GUID gEfiBlockIoProtocolGuid = { 0x964e5b21, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };
EFI_GUID gEfiDevicePathProtocolGuid = {0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }};
EFI_GUID gEfiGlobalVariableGuid = {0x8be4df61, 0x93ca, 0x11d2, {0xaa,0x0d,0x00,0xe0,0x98,0x03,0x2b,0x8c}};
EFI_GUID gEfiImageSecurityDatabaseGuid = {0xd719b2cb, 0x3d3a, 0x4596, {0xa3,0xbc,0xda,0xd0,0xe8,0x5b,0x12,0x7f}};

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
