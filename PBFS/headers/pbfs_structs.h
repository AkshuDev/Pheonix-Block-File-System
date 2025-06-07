#ifndef PBFS_STRUCTS_H
#define PBFS_STRUCTS_H

// We create structs for Pheonix Block File System

#include <unitypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "pbfs.h"

typedef struct {
    char Magic[6]; // PBFS\x00\x00
    uint32_t Block_Size; // 512 bytes
    uint32_t Total_Blocks;  // 2048 blocks
    char Disk_Name[24]; // Just the name of the disk
    uint64_t Timestamp; // Timestamp
    uint32_t Version; // Version
    uint64_t First_Boot_Timestamp; // First boot timestamp
    uint16_t OS_BootMode; // Again optional but there for furture use!
    uint32_t FileTableOffset; // Offset of the file table
    uint32_t Entries; // Number of entries in the file table
} __attribute__((packed)) PBFS_Header; // Total = 68 bytes

typedef struct {
    char Name[128]; // Name of the file
    uint128_t File_Data_Offset; // File data offset
    uint128_t Block_Span; // File Block Span
} __attribute__((packed)) PBFS_FileTableEntry; // Total = 164 bytes

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
} __attribute__((packed)) PBFS_PermisssionTableEntry; // Total = 16 bytes

typedef struct {
    char Name[128]; // Name of the file
} __attribute__((packed)) PBFS_FileTreeEntry; // Total = 128 bytes

// total size of all structs combined = 380 bytes
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

#endif