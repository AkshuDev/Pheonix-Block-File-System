#pragma once

#include <pbfs_structs.h>

uint128_t uint128_from_u32(uint32_t value);
uint128_t uint128_from_u64(uint64_t value);
uint32_t uint128_to_u32(uint128_t value);
uint64_t uint128_to_u64(uint128_t value);
void uint128_inc(uint128_t* a);
void uint128_dec(uint128_t* a);
void uint128_add(uint128_t* result, const uint128_t* a, const uint128_t* b);
void uint128_sub(uint128_t* result, const uint128_t* a, const uint128_t* b);
void uint128_mul_u32(uint128_t* result, const uint128_t* a, uint32_t b);
uint32_t uint128_div_u32(uint128_t* quotient, const uint128_t* dividend, uint32_t divisor);
int uint128_cmp(const uint128_t* a, const uint128_t* b);
int uint128_is_zero(const uint128_t* a);
uint64_t uint128_get_low64(const uint128_t* val);
int uint128_high64_is_zero(const uint128_t* val);
void uint128_inc_by(uint128_t* val, uint16_t count);

void bitmap_bit_set(uint8_t* bitmap, uint64_t bit_index);
void bitmap_bit_clear(uint8_t* bitmap, uint64_t bit_index);
int bitmap_bit_test(uint8_t* bitmap, uint64_t bit_index);
int is_lba_in_current_bitmap(uint128_t lba, uint128_t bitmap_index);

static const uint128_t zero128 = {{0, 0, 0, 0}};

#define UINT128_EQ(a, b) (uint128_cmp(&(a), &(b)) == 0)
#define UINT128_NEQ(a, b) (uint128_cmp(&(a), &(b)) != 0)
#define UINT128_GT(a, b) (uint128_cmp(&(a), &(b)) > 0)
#define UINT128_GTE(a, b) (uint128_cmp(&(a), &(b)) >= 0)
#define UINT128_LT(a, b) (uint128_cmp(&(a), &(b)) < 0)
#define UINT128_LTE(a, b) (uint128_cmp(&(a), &(b)) <= 0)

#define UINT128_ZERO zero128

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
    NoSpaceAvailable
} Errors;