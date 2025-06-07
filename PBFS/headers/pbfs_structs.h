#ifndef PBFS_STRUCTS_H
#define PBFS_STRUCTS_H

// We create structs for Pheonix Block File System

#include <unitypes.h>
#include <stdio.h>
#include <stdlib.h>

struct PBFS_Header {
    char Magic[6]; // PBFS\x00\x00
    uint32_t Block_Size; // 512 bytes
    uint32_t Total_Blocks;  // 2048 blocks
    char Disk_Name[24]; // Just the name of the disk
    uint32_t Timestamp_part1; // Total = 8 bytes but for compatibility with 32-bit cpu
    uint32_t Timestamp_part2; // Part 2
    uint32_t Version; // Version
    uint32_t First_Boot_Timestamp_Part1; // Total = 8 bytes but for compatibility with 32-bit cpu, also optional
    uint32_t First_Boot_Timestamp_Part2; // Part 2
    uint16_t OS_BootMode; // Again optional but there for furture use!
    uint32_t FileTableOffset; // Offset of the file table
    uint32_t Entries; // Number of entries in the file table
}; // Total = 68 bytes

struct PBFS_FileTableEntry {
    char Name[128]; // Name of the file
    uint32_t File_Offset_Part1; // Total = 16 bytes but for compatibility with 32-bit cpu
    uint32_t File_Offset_Part2; // Part 2
    uint32_t File_Offset_Part3; // Part 3
    uint32_t File_Offset_Part4; // Part 4
    uint32_t File_Data_Block_Span_Part1; // Total = 8 bytes but for compatibility with 32-bit cpu
    uint32_t File_Data_Block_Span_Part2; // Part 2
}; // Total = 156 bytes

struct PBFS_PermisssionTableEntry {
    uint16_t Read; // Read Permission
    uint16_t Write; // Write Permission
    uint16_t Executable; // Executable Permission
    uint16_t Listable; // Listable Permission
    uint16_t Hidden; // Hidden Permission
    uint16_t Full_Control; // System File Permission
    uint16_t Delete; // Delete Permission
    uint16_t Special_Access; // Special Access
    uint32_t File_Tree_Offset; // Offset of the file tree
}; // Total = 16 bytes

struct PBFS_FileTreeEntry {
    char Name[128]; // Name of the file
}; // Total = 128 bytes

// total size of all structs combined = 372

#endif