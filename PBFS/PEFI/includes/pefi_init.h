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
#include "pefi_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

// efi_init
LIB int InitalizeLib(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle);
LIB extern PEFI_InternalState pefi_state;

#ifdef __cplusplus
}
#endif