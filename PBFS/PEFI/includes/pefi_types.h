#ifndef PEFI_TYPES_H
#define PEFI_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* UEFI typedefs */
typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;
typedef uint16_t CHAR16;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;
typedef void VOID;
typedef uint64_t UINTN;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

/* Console Output Protocol */
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void* _pad;
    EFI_STATUS (*OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* self, const CHAR16* str);
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/* Boot Services (partial) */
typedef struct EFI_BOOT_SERVICES {
    char _pad1[24]; // Skip to AllocatePool
    EFI_STATUS (*AllocatePool)(UINTN pool_type, UINTN size, void** out);
    EFI_STATUS (*FreePool)(void* buffer);

    // You can add more: AllocatePages, LocateProtocol, etc.
} EFI_BOOT_SERVICES;

/* UEFI System Table */
typedef struct EFI_SYSTEM_TABLE {
    char _pad1[44]; // Skip unused entries
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    char _pad2[24]; // Skip other protocols
    EFI_BOOT_SERVICES* BootServices;
} EFI_SYSTEM_TABLE;

#endif
