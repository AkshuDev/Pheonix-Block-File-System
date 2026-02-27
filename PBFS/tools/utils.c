#include <pbfs-cli.h>
#include <pbfs_structs.h>
#include <pbfs.h>

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

void uint128_inc(uint128_t* a) {
    uint64_t carry = 1;
    for (int i = 0; i < 4 && carry != 0; i++) {
        uint64_t sum = (uint64_t)a->part[i] + carry;
        a->part[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
    }
}

void uint128_dec(uint128_t* a) {
    for (int i = 0; i < 4; i++) {
        if (a->part[i] == 0) {
            a->part[i] = 0xFFFFFFFF;
        } else {
            a->part[i]--;
            break;
        }
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

void uint128_sub(uint128_t* result, const uint128_t* a, const uint128_t* b) {
    uint64_t borrow = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t sub = (uint64_t)a->part[i] - b->part[i] - borrow;
        result->part[i] = (uint32_t)(sub & 0xFFFFFFFF);
        borrow = (sub >> 32) & 1;
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

// Divide by uint32_t
uint32_t uint128_div_u32(uint128_t* quotient, const uint128_t* dividend, uint32_t divisor) {
    uint64_t remainder = 0;
    for (int i = 3; i >= 0; i--) {
        uint64_t current = (remainder << 32) | dividend->part[i];
        quotient->part[i] = (uint32_t)(current / divisor);
        remainder = current % divisor;
    }
    return (uint32_t)remainder;
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

void bitmap_bit_set(uint8_t* bitmap, uint64_t bit_index) {
    uint64_t byte_idx = bit_index / 8;
    if (byte_idx >= PBFS_BITMAP_LIMIT) return;
    uint8_t bit_offset = bit_index % 8;
    bitmap[byte_idx] |= (1 << bit_offset);
}

void bitmap_bit_clear(uint8_t* bitmap, uint64_t bit_index) {
    uint64_t byte_idx = bit_index / 8;
    if (byte_idx >= PBFS_BITMAP_LIMIT) return;
    uint8_t bit_offset = bit_index % 8;
    bitmap[byte_idx] &= ~(1 << bit_offset);
}

// 1 = set, 0 = not set/cleared, -1 error
int bitmap_bit_test(uint8_t* bitmap, uint64_t bit_index) {
    uint64_t byte_idx = bit_index / 8;
    if (byte_idx >= PBFS_BITMAP_LIMIT) return -1;
    uint8_t bit_offset = bit_index % 8;
    return (bitmap[byte_idx] >> bit_offset) & 1;
}

int is_lba_in_current_bitmap(uint128_t lba, uint128_t bitmap_index) {
    uint128_t range_start = UINT128_ZERO;
    uint128_mul_u32(&range_start, &bitmap_index, (PBFS_BITMAP_LIMIT * 8));
    uint128_t range_end = UINT128_ZERO;
    uint128_t max_bitmap_block_range = uint128_from_u32(PBFS_BITMAP_LIMIT * 8);
    uint128_add(&range_end, &range_start, &max_bitmap_block_range);
    return (UINT128_GTE(lba, range_start) && UINT128_LT(lba, range_end));
}

// some statics 
static int pbfs_strlen(const char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

static void pbfs_strcpy(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

static void pbfs_memcpy(char* dst, const char* src, int n) {
    for (int i = 0; i < n; i++)
        dst[i] = src[i];
}

// Path features and stuff
void path_normalize(char* path, char* out, int out_size) {
    int i = 0;
    int o = 0;

    if (out_size <= 0)
        return;

    // '~' expansion
    if (path[0] == '~') {
        const char* home = "/root";
        int h = 0;

        while (home[h] && o < out_size - 1) {
            out[o++] = home[h++];
        }

        i = 1; // skip '~'
    }

    int prev_was_slash = 0;

    while (path[i] && o < out_size - 1) {
        if (path[i] == '/') {
            if (!prev_was_slash) {
                out[o++] = '/';
                prev_was_slash = 1;
            }
        } else {
            out[o++] = path[i];
            prev_was_slash = 0;
        }
        i++;
    }

    // Remove trailing slash (except root)
    if (o > 1 && out[o - 1] == '/')
        o--;

    out[o] = 0;
}

void path_dirname(char* path, char* out, int out_size) {
    int len = pbfs_strlen(path);

    if (len == 0) {
        if (out_size > 0) {
            out[0] = '/';
            if (out_size > 1) out[1] = 0;
        }
        return;
    }

    int last = -1;
    for (int i = 0; i < len; i++) {
        if (path[i] == '/')
            last = i;
    }

    if (last == -1) {
        if (out_size > 0) {
            out[0] = '/'; 
            if (out_size > 1) out[1] = 0;
        }
        return;
    }

    if (last == 0) {
        if (out_size > 0) {
            out[0] = '/';
            if (out_size > 1) out[1] = 0;
        }
        return;
    }

    int copy_len = (last < out_size - 1) ? last : out_size - 1;

    for (int i = 0; i < copy_len; i++)
        out[i] = path[i];

    out[copy_len] = 0;
}

void path_basename(char* path, char* out, int out_size) {
    int len = pbfs_strlen(path);

    if (len == 0) {
        if (out_size > 0) out[0] = 0;
        return;
    }

    // Skip trailing slashes
    int end = len - 1;
    while (end > 0 && path[end] == '/')
        end--;

    int start = end;

    while (start > 0 && path[start - 1] != '/')
        start--;

    int part_len = end - start + 1;
    if (part_len >= out_size)
        part_len = out_size - 1;

    for (int i = 0; i < part_len; i++)
        out[i] = path[start + i];

    out[part_len] = 0;
}

void path_part(char* path, int index, char* out, int out_size) {
    int len = pbfs_strlen(path);
    int current_part = 0;
    int i = 0;

    // Skip leading slashes
    while (path[i] == '/')
        i++;

    while (i < len) {
        int start = i;

        while (i < len && path[i] != '/')
            i++;

        int end = i;

        if (current_part == index) {
            int part_len = end - start;
            if (part_len >= out_size)
                part_len = out_size - 1;

            for (int j = 0; j < part_len; j++)
                out[j] = path[start + j];

            out[part_len] = 0;
            return;
        }

        current_part++;

        while (path[i] == '/')
            i++;
    }

    if (out_size > 0)
        out[0] = 0;
}