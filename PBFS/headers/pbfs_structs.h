#ifndef PBFS_STRUCTS_H
#define PBFS_STRUCTS_H

// We create structs for Pheonix Block File System

#include <unitypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "pbfs.h"

#pragma pack(push, 1)
typedef struct {
    char Magic[6]; // PBFS\x00\x00
    uint32_t Block_Size; // 512 bytes by default (BIOS BLOCK SIZE)
    uint32_t Total_Blocks;  // 2048 blocks for 1MB
    char Disk_Name[24]; // Just the name of the disk for the os
    uint64_t Timestamp; // Timestamp
    uint32_t Version; // Version
    uint64_t First_Boot_Timestamp; // First boot timestamp
    uint16_t OS_BootMode; // Again optional but there for furture use!
    uint32_t FileTableOffset; // Offset of the file table (first data block)
    uint32_t Entries; // Number of entries in the file table
} __attribute__((packed)) PBFS_Header; // Total = 68 bytes
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    char Name[128]; // Name of the file
    uint128_t File_Data_Offset; // File data offset
    uint128_t Permission_Table_Offset; // Permission table offset
    uint128_t Block_Span; // File Block Span
} __attribute__((packed)) PBFS_FileTableEntry; // Total = 176 bytes
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t Read; // Read Permission
    uint16_t Write; // Write Permission
    uint16_t Executable; // Executable Permission
    uint16_t Listable; // Listable Permission
    uint16_t Hidden; // Hidden Permission
    uint16_t Full_Control; // System File Permission
    uint16_t Delete; // Delete Permission
    uint16_t Special_Access; // Special Access
    uint32_t File_Tree_Offset; // Offset of the file tree
} __attribute__((packed)) PBFS_PermissionTableEntry; // Total = 20 bytes
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    char Name[20]; // Name of the file (unique id is also accepted, it is 20 bytes because the file paths are not used here but rather the unique id is used).
} __attribute__((packed)) PBFS_FileTreeEntry; // Total = 20 bytes
#pragma pack(pop)

// total size of all structs combined = 392 bytes
#pragma pack(push, 1)
typedef struct {
    uint8_t size;
    uint8_t reserved;
    uint16_t sector_count;
    uint16_t offset;
    uint16_t segment;
    uint64_t lba;
} __attribute__((packed)) PBFS_DAP;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t Read; // Read Permission
    uint16_t Write; // Write Permission
    uint16_t Executable; // Executable Permission
    uint16_t Listable; // Listable Permission
    uint16_t Hidden; // Hidden Permission
    uint16_t Full_Control; // System File Permission
    uint16_t Delete; // Delete Permission
    uint16_t Special_Access; // Special Access
} __attribute__((packed)) PBFS_Permissions; // Total = 16 bytes
#pragma pack(pop)

#pragma pack(push, 1)
// PBFS_Layout
typedef struct {
    uint64_t Header_Start;
    uint64_t Header_End;
    uint64_t Header_BlockSpan;
    uint64_t Bitmap_Start;
    uint64_t Bitmap_BlockSpan;
    uint64_t Bitmap_End;
    uint64_t Data_Start;
} __attribute__((packed)) PBFS_Layout;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t size;              // 0x00 - Must be 0x1E (30)
    uint16_t flags;             // 0x02
    uint32_t cylinders;         // 0x04 - reserved or zero
    uint32_t heads;             // 0x08 - reserved or zero
    uint32_t sectors_per_track; // 0x0C - reserved or zero
    uint64_t total_sectors;     // 0x10 - important!
    uint16_t bytes_per_sector;  // 0x18 - important!
    uint8_t  reserved[6];       // 0x1A - reserved
} __attribute__((packed)) DriveParameters;
#pragma pack(pop)

typedef struct {
    char* name;
    uint32_t lba;
} PBFS_FileListEntry;

#endif