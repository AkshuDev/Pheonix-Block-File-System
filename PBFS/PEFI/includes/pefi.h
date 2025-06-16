#ifndef PEFI_H
#define PEFI_H

#include <efi.h>
#include <efilib.h>

// Forward declare for each module
struct EFI_GRAPHICS_CONTEXT;
struct EFI_DISK_CONTEXT;
struct EFI_MEMORY_CONTEXT;

// Pheonix EFI unified interface
typedef struct {
    EFI_HANDLE ImageHandle;
    EFI_SYSTEM_TABLE* SystemTable;

    struct EFI_GRAPHICS_CONTEXT* Graphics;
    struct EFI_DISK_CONTEXT* Disk;
    struct EFI_MEMORY_CONTEXT* Memory;
} PEFI;
