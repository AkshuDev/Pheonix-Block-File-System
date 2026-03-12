#pragma once

// LIB
#define LIB EFIAPI

#include "pefi_types.h"
#include "pefi.h"

#ifdef __cplusplus
extern "C" {
#endif

// main.c
LIB void pefi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str) __attribute__((used));
LIB EFI_STATUS pefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) __attribute__((used));
LIB void pefi_clear(EFI_SYSTEM_TABLE* SystemTable) __attribute__((used));
LIB void pefi_enable_cursor(EFI_SYSTEM_TABLE* SystemTable) __attribute__((used));
LIB void pefi_disable_cursor(EFI_SYSTEM_TABLE* SystemTable) __attribute__((used));

#ifdef __cplusplus
}
#endif