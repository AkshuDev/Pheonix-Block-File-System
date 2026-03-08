#pragma once

#include "pbfs_structs.h"

uint128_t uint128_from_u32(uint32_t value) __attribute__((used));
uint128_t uint128_from_u64(uint64_t value) __attribute__((used));
uint32_t uint128_to_u32(uint128_t value) __attribute__((used));
uint64_t uint128_to_u64(uint128_t value) __attribute__((used));
void uint128_inc(uint128_t* a) __attribute__((used));
void uint128_dec(uint128_t* a) __attribute__((used));
void uint128_add(uint128_t* result, const uint128_t* a, const uint128_t* b) __attribute__((used));
void uint128_sub(uint128_t* result, const uint128_t* a, const uint128_t* b) __attribute__((used));
void uint128_mul_u32(uint128_t* result, const uint128_t* a, uint32_t b) __attribute__((used));
uint32_t uint128_div_u32(uint128_t* quotient, const uint128_t* dividend, uint32_t divisor) __attribute__((used));
int uint128_cmp(const uint128_t* a, const uint128_t* b) __attribute__((used));
int uint128_is_zero(const uint128_t* a) __attribute__((used));
uint64_t uint128_get_low64(const uint128_t* val) __attribute__((used));
int uint128_high64_is_zero(const uint128_t* val) __attribute__((used));
void uint128_inc_by(uint128_t* val, uint16_t count) __attribute__((used));

void bitmap_bit_set(uint8_t* bitmap, uint64_t bit_index) __attribute__((used));
void bitmap_bit_clear(uint8_t* bitmap, uint64_t bit_index) __attribute__((used));
int bitmap_bit_test(uint8_t* bitmap, uint64_t bit_index) __attribute__((used));
int is_lba_in_current_bitmap(uint128_t lba, uint128_t bitmap_index) __attribute__((used));

void path_normalize(char* path, char* out, int out_size) __attribute__((used));
void path_dirname(char* path, char* out, int out_size) __attribute__((used));
void path_basename(char* path, char* out, int out_size) __attribute__((used));
void path_part(char* path, int index, char* out, int out_size) __attribute__((used));
void path_join(char* out, char* p1, char* p2, int out_size) __attribute__((used));

static const uint128_t zero128 = {{0, 0, 0, 0}};

#define UINT128_EQ(a, b) (uint128_cmp(&(a), &(b)) == 0)
#define UINT128_NEQ(a, b) (uint128_cmp(&(a), &(b)) != 0)
#define UINT128_GT(a, b) (uint128_cmp(&(a), &(b)) > 0)
#define UINT128_GTE(a, b) (uint128_cmp(&(a), &(b)) >= 0)
#define UINT128_LT(a, b) (uint128_cmp(&(a), &(b)) < 0)
#define UINT128_LTE(a, b) (uint128_cmp(&(a), &(b)) <= 0)

#define UINT128_ZERO zero128

// Errors
#ifdef PBFS_CLI
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
#endif
