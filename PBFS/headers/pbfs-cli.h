#ifndef PBFS_H
#define PBFS_H

#include <unitypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#define MEM_MAGIC "PBFS\0\0"
#define MEM_END_MAGIC "PBFS_END\0\0"
#define MEM_MAGIC_LEN 6
#define MEM_END_MAGIC_LEN 10

#pragma pack(push, 1)
typedef struct {
    uint32_t part[4]; // [0] = LSB, [3] = MSB
} uint128_t;
#pragma pack(pop)

uint128_t uint128_from_u32(uint32_t value) {
    uint128_t out = {{ value, 0, 0, 0 }};
    return out;
}
uint128_t uint128_from_u64(uint64_t value) {
    uint128_t out = {{ (uint32_t)(value & 0xFFFFFFFF), (uint32_t)(value >> 32), 0, 0 }};
    return out;
}

uint32_t uint128_to_u32(uint128_t value) {
    return value.part[0];
}

uint64_t uint128_to_u64(uint128_t value) {
    return ((uint64_t)value.part[1] << 32) | value.part[0];
}

uint128_t uint128_inc(uint128_t *a) {
    uint64_t carry = 1;
    for (int i = 0; i < 4 && carry != 0; i++) {
        uint64_t sum = (uint64_t)a->part[i] + carry;
        a->part[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
    }
}

void uint128_add(uint128_t* result, const uint128_t* a, const uint128_t* b) {
    uint64_t carry = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t sum = (uint64_t)a->part[i] + b->part[i] + carry;
        result->part[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
    }
}

// Multiply by uint32_t
void uint128_mul_u32(uint128_t* result, const uint128_t* a, uint32_t b) {
    uint64_t carry = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t prod = (uint64_t)a->part[i] * b + carry;
        result->part[i] = (uint32_t)(prod & 0xFFFFFFFF);
        carry = prod >> 32;
    }
}

// Compare a and b
int uint128_cmp(const uint128_t* a, const uint128_t* b) {
    for (int i = 3; i >= 0; i--) {
        if (a->part[i] < b->part[i]) return -1;
        if (a->part[i] > b->part[i]) return 1;
    }
    return 0;
}

// Zero check
int uint128_is_zero(const uint128_t* a) {
    return a->part[0] == 0 && a->part[1] == 0 && a->part[2] == 0 && a->part[3] == 0;
}

// Debug print
void uint128_print(const uint128_t* a) {
    printf("0x%08X%08X%08X%08X\n", a->part[3], a->part[2], a->part[1], a->part[0]);
}

uint64_t uint128_get_low64(const uint128_t* val) {
    return ((uint64_t)val->part[1] << 32) | val->part[0];
}

int uint128_high64_is_zero(const uint128_t* val) {
    return val->part[3] == 0 && val->part[2] == 0;
}

void uint128_inc_by(uint128_t* val, uint16_t count) {
    // Increment by count (safely handles carry)
    uint64_t low = uint128_get_low64(val);
    low += count;
    val->part[0] = (uint32_t)(low & 0xFFFFFFFF);
    val->part[1] = (uint32_t)(low >> 32);
    // Add carry if overflowed (you can use full adder logic)
}

const uint128_t zero128 = {{0, 0, 0, 0}};

#define UINT128_EQ(a, b) (uint128_cmp(&(a), &(b)) == 0)
#define UINT128_ZERO zero128

#ifdef _WIN32
    #define CALLCONV __cdecl
#else
    #define CALLCONV
#endif

// Errors
typedef enum {
    //General
    Unkown,
    Failure,
    PermissionDenied,
    // Usage and Arguments
    InvalidUsage,
    InvalidArgument,
    InvalidPath,
    UnknownArgument,
    // PBFS Related
    InvalidHeader,
    // Disk
    DiskError,
    DiskCorrupted,
    DiskNotFound,
    DiskNotReadable,
    DiskNotWritable,
    DiskNotFormatted,
    // File
    FileError,
    FileNotFound,
    FileAlreadyExists,
    FileNotReadable,
    FileNotWritable,
    FileNotExecutable,
    FileNotListable,
    FileNotHidden,
    FileNotFullControl,
    FileNotDelete,
    FileNotSpecialAccess,
    FileCannotBeDeleted,
    FileCannotBeMoved,
    FileCannotBeCopied,
    FileCannotBeRenamed,
    FileCannotBeTruncated,
    FileCannotBeCreated,
    // Header
    HeaderVerificationFailed,
    // Allocation
    AllocFailed,
    // Memory
    NoMemoryAvailable,
} Errors;

#define PBFS_CLI_HELP "Usage: pbfs-cli <image> <commands>\nCommands:\n\t-bs <size>: Define the Block Size\n\t-tb <count>: Define Total number of blocks\n\t-dn <name>: Define the disk name\n\t-f: Format the disk\n\t-c: Create the disk\n\t-add <filename> <filepath>: Adds a file to the image.\n\t-bootloader <filepath>: Adds a bootloader to the image (MBR).\n\t-partboot <Number of Blocks>: Adds a Boot partition of size Block Size times Span specified here.\n\t-partkernel: Reserves a Kernel Entry block\n\t-kernel <path>: Adds a kernel with no metadata and adds it to the kernel entry if found.\n"

#endif
