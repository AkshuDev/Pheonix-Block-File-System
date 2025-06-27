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

// main.c
LIB void pefi_print(EFI_SYSTEM_TABLE* SystemTable, const uint16_t* str);
LIB EFI_STATUS pefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

#ifdef __cplusplus
}
#endif