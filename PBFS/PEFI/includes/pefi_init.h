#pragma once

// LIB
#define LIB

#include "pefi_types.h"
#include "pefi.h"
#include "pefi_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

// efi_init
LIB int InitalizeLib(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) __attribute__((used));
LIB extern PEFI_InternalState pefi_state;

#ifdef __cplusplus
}
#endif