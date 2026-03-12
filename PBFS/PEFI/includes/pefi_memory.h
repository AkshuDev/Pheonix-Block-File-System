#pragma once

// LIB
#define LIB EFIAPI

#include "pefi_types.h"
#include "pefi.h"

#ifdef __cplusplus
extern "C" {
#endif

// efi_memory
LIB EFI_STATUS EFIAPI pefi_allocate(EFI_SYSTEM_TABLE *SystemTable, UINTN size, void** ptr) __attribute__((used));
LIB EFI_STATUS EFIAPI pefi_free(EFI_SYSTEM_TABLE* SystemTable, void* ptr) __attribute__((used));

#ifdef __cplusplus
}
#endif