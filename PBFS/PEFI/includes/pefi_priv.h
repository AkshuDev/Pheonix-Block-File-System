#pragma once

#include <pefi.h>

#define PEFI_VERSION 1
#define PEFI_ALREADY_INITIALIZED 100


// Library state structure
typedef struct {
    uint64_t version;
    uint32_t initialized;
    EFI_SYSTEM_TABLE* system_table;  // Pointer to EFI system table (if needed)
    EFI_BOOT_SERVICES* boot_services; // Pointer to EFI boot services (if needed)
    EFI_RUNTIME_SERVICES* runtime_services; // Pointer to EFI runtime services (if needed)
    EFI_HANDLE* image_handle; // Pointer to EFI image handle
} PEFI_InternalState;

// Global library state (internal use only)
extern PEFI_InternalState pefi_state;