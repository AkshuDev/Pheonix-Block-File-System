#pragma once

// LIB
#ifdef _WIN32
// DLL MODE
#ifdef BUILD_LIB
#define LIB __declspec(dllexport)
#else
#define LIB __declspec(dllimport)
#endif
#else
// .SO MODE
#ifdef BUILD_LIB
#define LIB __attribute__((visibility("default")))
#else
#define LIB
#endif
#endif

#include "pefi_types.h"
#include "pefi.h"

#ifdef __cplusplus
extern "C" {
#endif

// efi_memory
LIB EFI_STATUS EFIAPI pefi_allocate(EFI_SYSTEM_TABLE *SystemTable, UINTN size, void** ptr);
LIB EFI_STATUS EFIAPI pefi_free(EFI_SYSTEM_TABLE* SystemTable, void* ptr);

#ifdef __cplusplus
}
#endif