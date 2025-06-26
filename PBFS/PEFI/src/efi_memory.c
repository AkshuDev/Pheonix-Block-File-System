#include "pefi_types.h"
#define BUILD_LIB
#include <pefilib.h>

#define EfiReservedMemoryType       0
#define EfiLoaderCode               1
#define EfiLoaderData               2
#define EfiBootServicesCode         3
#define EfiBootServicesData         4
#define EfiRuntimeServicesCode      5
#define EfiRuntimeServicesData      6
#define EfiConventionalMemory       7
#define EfiUnusableMemory           8
#define EfiACPIReclaimMemory        9
#define EfiACPIMemoryNVS            10
#define EfiMemoryMappedIO           11
#define EfiMemoryMappedIOPortSpace  12


LIB EFI_STATUS EFIAPI Allocate(EFI_SYSTEM_TABLE* SystemTable, UINTN size, void** ptr) {
    return SystemTable->BootServices->AllocatePool(2 /* EfiLoaderData */, size, ptr);
}

LIB EFI_STATUS EFIAPI Free(EFI_SYSTEM_TABLE* SystemTable, void* ptr) {
    return SystemTable->BootServices->FreePool(ptr);
}
