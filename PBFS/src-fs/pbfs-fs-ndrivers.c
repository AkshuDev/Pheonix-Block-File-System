#include <pbfs.h>
#include <pbfs_structs.h>
#include <pbfs_structs_64.h>

#define PBFS_NDRIVERS
#ifdef PBFS_WDRIVERS
    #undef PBFS_WDRIVERS
#endif

#include <pbfs-fs.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

// Bootloader structs
struct btl_mbr_parition_entry {
    uint8_t bootable_flag; // 0x80 = Active/bootable, 0x00 = Not active
    uint8_t start_chs[3]; // Starting CHS address
    uint8_t partition_type; // File system indicator
    uint8_t end_chs[3]; // Ending CHS address
    uint32_t start_lba; // Starting Logical Block Addressing (LBA) address
    uint32_t size_sectors; // Partition length in sectors
} __attribute__((packed));

struct btl_mbr {
    uint8_t bootstrap_code[446];
    struct btl_mbr_parition_entry partition_table[4];
    uint16_t mbr_signature;
} __attribute__((packed));

struct btl_gpt_hdr {
    char signature[8];
    uint32_t revision;
    
    uint32_t header_size;
    uint32_t header_crc32;
    
    uint32_t reserved;

    uint64_t mylba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    
    uint8_t disk_guid[16];

    uint64_t partition_entry_lba;
    uint32_t number_of_partition_entries;
    uint32_t size_of_partition_entries;
    uint32_t partition_entry_array_crc32;

    uint8_t reserved2[420];
} __attribute__((packed));

struct btl_gpt_entry {
    uint8_t partition_type_guid[16];
    uint8_t unique_partition_guid[16];

    uint64_t starting_lba;
    uint64_t ending_lba;
    
    uint64_t attributes;

    uint16_t partition_name[36];
    
    uint8_t reserved[128];
} __attribute__((packed));

struct guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} __attribute__((packed));

static struct pbfs_funcs funcs = {0};

static uint32_t crc32_table[256];
static int crc32_initialized = 0;
static uint64_t guid_seed = 0x123456789ABCDEF0;

// static funcs
static void crc32_init() {
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (size_t j = 0; j < 8; j++) {
            if (c & 1) c = polynomial ^ (c >> 1);
            else c >>= 1;
        }
        crc32_table[i] = c;
    }
    crc32_initialized = 1;
}

static uint32_t crc32(const void *data, size_t len) {
    if (!crc32_initialized) crc32_init();
    uint32_t c = 0xFFFFFFFF;
    const uint8_t *u8 = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        c = crc32_table[(c ^ u8[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

void generate_guid(uint8_t *guid) {
    static uint64_t state = 0;
    if (state == 0) {
        // Seed with the CPU timestamp
        uint32_t low, high;
        asm volatile ("rdtsc" : "=a"(low), "=d"(high));
        state = ((uint64_t)high << 32) | low;
    }

    // Generate 16 bytes of pseudo-random data
    for (int i = 0; i < 16; i++) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        guid[i] = (uint8_t)(state & 0xFF);
    }

    // Set Version 4 (Random) bits
    guid[6] = (guid[6] & 0x0F) | 0x40;
    // Set Variant (RFC 4122) bits
    guid[8] = (guid[8] & 0x3F) | 0x80;
}

static bool validate_hdr_magic(PBFS_Header* hdr) {
    if (memcmp(hdr->magic, PBFS_MAGIC, PBFS_MAGIC_LEN) != 0) return false;
    return true;
}

static void hdr_to_hdr64(PBFS_Header* src, PBFS_Header64* dst) {
    memcpy(&dst->magic, &src->magic, PBFS_MAGIC_LEN);
    
    dst->volume_id = uint128_to_u64(src->volume_id);

    dst->bitmap_lba = src->bitmap_lba;
    dst->sysinfo_lba = src->sysinfo_lba;
    dst->kernel_table_lba = src->kernel_table_lba;
    dst->dmm_root_lba = src->dmm_root_lba;
    dst->boot_partition_size = src->boot_partition_size;
    dst->boot_partition_lba = src->boot_partition_lba;
    dst->data_start_lba = src->data_start_lba;

    dst->total_blocks = uint128_to_u64(src->total_blocks);
    dst->block_size = src->block_size;

    memcpy(&dst->disk_name, &src->disk_name, PBFS_DISK_NAME_LEN);
}

static void bitmap_to_bitmap64(PBFS_Bitmap* src, PBFS_Bitmap64* dst) {
    memcpy(&dst->bytes, &src->bytes, sizeof(src->bytes));
    dst->extender_lba = uint128_to_u64(src->extender_lba);
}

static void dmmentry_to_dmmentry64(PBFS_DMM_Entry* src, PBFS_DMM_Entry64* dst) {
    memcpy(&dst->name, &src->name, PBFS_MAX_NAME_LEN);
    dst->lba = uint128_to_u64(src->lba);
    dst->type = src->type;
    dst->perms = src->perms;
    dst->created_timestamp = src->created_timestamp;
    dst->modified_timestamp = src->modified_timestamp;
}

static void dmm_to_dmm64(PBFS_DMM* src, PBFS_DMM64* dst) {
    for (uint64_t i = 0; i < src->entry_count; i++) {
        dmmentry_to_dmmentry64(&src->entries[i], &dst->entries[i]);
    }
    dst->entry_count = src->entry_count;
    dst->extender_lba = uint128_to_u64(src->extender_lba);
}

static void kernelentry_to_kernelentry64(PBFS_Kernel_Entry* src, PBFS_Kernel_Entry64* dst) {
    memcpy(&dst->name, &src->name, PBFS_MAX_NAME_LEN);
    dst->lba = uint128_to_u64(src->lba);
}

static void kernel_table_to_kernel_table64(PBFS_Kernel_Table* src, PBFS_Kernel_Table64* dst) {
    for (uint64_t i = 0; i < src->entry_count; i++) {
        kernelentry_to_kernelentry64(&src->entries[i], &dst->entries[i]);
    }
    dst->entry_count = src->entry_count;
    dst->extender_lba = uint128_to_u64(src->extender_lba);
}

static int bitmap_test_recursive(PBFS_Bitmap64* current, uint64_t lba, uint64_t bitmap_idx, struct pbfs_mount* mnt) {
    if (is_lba_in_current_bitmap(uint128_from_u64(lba), uint128_from_u64(bitmap_idx))) {
        uint64_t local_bit = lba % (PBFS_BITMAP_LIMIT * 8);
        return bitmap_bit_test(current->bytes, local_bit);
    }

    uint64_t ext_lba = current->extender_lba;
    if (ext_lba <= 1) return -1;

    PBFS_Bitmap next = {0};
    PBFS_Bitmap64 next64 = {0};
    if (pbfs_read(mnt, ext_lba, sizeof(PBFS_Bitmap), &next) != PBFS_RES_SUCCESS) return 0;
    bitmap_to_bitmap64(&next, &next64);

    return bitmap_test_recursive(&next64, lba, bitmap_idx + 1, mnt);
}

static uint64_t bitmap_find_blocks(PBFS_Bitmap64* start, uint64_t start_lba, uint64_t blocks, struct pbfs_mount* mnt) {
    if (blocks == 0) return 0;
    
    PBFS_Bitmap64 current = *start;
    PBFS_Bitmap tmp_bitmap = {0};
    uint64_t bitmap_idx = 0;

    uint64_t run_start = 0;
    uint64_t run_length = 0;
    bool in_run = false;

    for (uint64_t depth = 0; depth < PBFS_MAX_BITMAP_CHAIN; depth++) {
        uint64_t slba = depth == 0 ? start_lba : 0;
        for (uint64_t i = slba; i < (PBFS_BITMAP_LIMIT * 8); i++) {

            if (!bitmap_bit_test(current.bytes, i)) {
                if (!in_run) {
                    run_start = (bitmap_idx * (PBFS_BITMAP_LIMIT * 8)) + i;
                    run_length = 0;
                    in_run = true;
                }
                run_length++;
                if (run_length == blocks)
                    return run_start;
            } else {
                in_run = false;
                run_length = 0;
            }
        }
        if (current.extender_lba <= 1)
            return 0;

        if (pbfs_read(mnt, current.extender_lba, sizeof(PBFS_Bitmap), &tmp_bitmap) != PBFS_RES_SUCCESS) return 0;
        bitmap_to_bitmap64(&tmp_bitmap, &current);

        bitmap_idx++;
    }

    return 0;
}

static uint64_t bitmap_set(PBFS_Bitmap* current, uint64_t lba, uint64_t bitmap_lba, uint64_t bitmap_idx, struct pbfs_mount* mnt, uint8_t value) { // value: 1/0 (default: 0)
    PBFS_Bitmap64 current64 = {0};
    bitmap_to_bitmap64(current, &current64);
    
    if (is_lba_in_current_bitmap(uint128_from_u64(lba), uint128_from_u64(bitmap_idx))) {
        uint64_t local_bit = lba % (PBFS_BITMAP_LIMIT * 8);
        if (value == 1)
            bitmap_bit_set(current->bytes, local_bit);
        else
            bitmap_bit_clear(current->bytes, local_bit);
        if (pbfs_write(mnt, bitmap_lba, sizeof(PBFS_Bitmap), current) != PBFS_RES_SUCCESS) return 0;
        return lba;
    }

    // Need next extender
    PBFS_Bitmap next = {0};
    PBFS_Bitmap64 next64 = {0};

    if (current64.extender_lba <= 1) {
        // Find a free block in CURRENT bitmap to use as the NEW extension block
        // use the first available bit in the current data range
        uint64_t new_block = 0;
        bool found = false;
        for (uint64_t i = 0; i < (PBFS_BITMAP_LIMIT * 8); i++) {
            if (!bitmap_bit_test(current->bytes, i)) {
                new_block = (bitmap_idx * (PBFS_BITMAP_LIMIT * 8)) + i;
                bitmap_bit_set(current->bytes, i);
                found = true;
                break;
            }
        }

        if (found == false) return 0; // Truly out of space

        // Link current to new
        current64.extender_lba = new_block;
        current->extender_lba = uint128_from_u64(new_block);
        if (pbfs_write(mnt, bitmap_lba, sizeof(PBFS_Bitmap), current) != PBFS_RES_SUCCESS) return 0;

        // Initialize new bitmap block
        memset(&next, sizeof(PBFS_Bitmap), 0);
        if (pbfs_write(mnt, new_block, sizeof(PBFS_Bitmap), &next) != PBFS_RES_SUCCESS) return 0;
    } else {
        if (pbfs_read(mnt, current64.extender_lba, sizeof(PBFS_Bitmap), &next) != PBFS_RES_SUCCESS) return 0;
        bitmap_to_bitmap64(&next, &next64);
    }

    return bitmap_set(&next, lba, current64.extender_lba, ++bitmap_idx, mnt, value);
}

static int add_dmm_entry(struct pbfs_mount* mnt, PBFS_DMM* cur_dmm, uint64_t cur_dmm_lba, uint64_t lba, char* name, PBFS_Metadata_Flags type, PBFS_Permission_Flags perms, uint64_t created_ts, uint64_t modified_ts) {
    PBFS_DMM dmm = *cur_dmm;
    PBFS_DMM64 dmm64 = {0};
    dmm_to_dmm64(&dmm, &dmm64);
    PBFS_DMM* current = &dmm;
    PBFS_DMM64* current64 = &dmm64;
    uint64_t cur_lba = cur_dmm_lba;

    for (int depth = 0; depth < PBFS_MAX_DMM_CHAIN; depth++) {
        // If there is space in current DMM, add the entry
        if (current->entry_count < PBFS_DMM_ENTRIES) {
            PBFS_DMM_Entry* entry = &current->entries[current->entry_count];
            entry->lba = uint128_from_u64(lba);
            entry->type = type;
            entry->created_timestamp = created_ts;
            entry->modified_timestamp = modified_ts;
            entry->perms = perms;
            strncpy(entry->name, name, sizeof(entry->name) - 1);
            entry->name[sizeof(entry->name) - 1] = '\0';
            current->entry_count++;

            return pbfs_write(mnt, cur_lba, sizeof(PBFS_DMM), current);
        }

        // If current DMM is full, check for extender
        if (current64->extender_lba > 0) {
            cur_lba = current64->extender_lba;
            int out = pbfs_read(mnt, cur_lba, sizeof(PBFS_DMM), &dmm);
            if (out != PBFS_RES_SUCCESS) return out;
            continue;
        }

        // Need to allocate a new extender block
        PBFS_DMM new_extender = {0};
        new_extender.entry_count = 1;
        PBFS_DMM_Entry* entry = &new_extender.entries[0];
        entry->lba = uint128_from_u64(lba);
        entry->type = type;
        entry->created_timestamp = created_ts;
        entry->modified_timestamp = modified_ts;
        entry->perms = perms;
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';

        uint64_t new_ext_lba = bitmap_find_blocks(&mnt->root_bitmap64, mnt->header64.data_start_lba, 1, mnt);
        if (new_ext_lba == 0) return PBFS_ERR_No_Space_Left;

        // Write new extender to disk
        int out = pbfs_write(mnt, new_ext_lba, sizeof(PBFS_DMM), &new_extender);
        if (out != PBFS_RES_SUCCESS) return out;
        bitmap_set(&mnt->root_bitmap, new_ext_lba, mnt->header64.bitmap_lba, 0, mnt, 1);

        // Link current DMM to extender
        current->extender_lba = uint128_from_u64(new_ext_lba);

        out = pbfs_write(mnt, cur_lba, sizeof(PBFS_DMM), &current);
        if (out != PBFS_RES_SUCCESS) return out;

        return PBFS_RES_SUCCESS;
    }

    return PBFS_ERR_No_Space_Left;
}

static int remove_dmm_entry(struct pbfs_mount* mnt, char* name_, uint64_t cur_dmm_lba) {
    PBFS_DMM dmm = {0};

    uint64_t current_lba = cur_dmm_lba;
    char name[PBFS_MAX_NAME_LEN];
    path_basename(name_, name, PBFS_MAX_NAME_LEN);

    for (int depth = 0; depth < PBFS_MAX_DMM_CHAIN; depth++) {
        int out = pbfs_read(mnt, current_lba, sizeof(PBFS_DMM), &dmm);
        if (out != PBFS_RES_SUCCESS) return out;

        int found_idx = -1;
        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (strcmp(dmm.entries[i].name, name) == 0) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            // Shift entries left to fill the gap
            for (uint32_t i = found_idx; i < dmm.entry_count - 1; i++) {
                dmm.entries[i] = dmm.entries[i + 1];
            }
            dmm.entry_count--;

            // Write updated DMM block
            return pbfs_write(mnt, current_lba, sizeof(PBFS_DMM), &dmm);
        }

        if (UINT128_GT(dmm.extender_lba, UINT128_ZERO)) {
            current_lba = uint128_to_u64(dmm.extender_lba);
        } else {
            break;
        }
    }

    return PBFS_ERR_File_Not_Found;
}

static int find_dmm_entry(const char* path, PBFS_DMM_Entry* out, uint64_t* out_lba, struct pbfs_mount* mnt) {
    char normalized[PBFS_MAX_PATH_LEN];
    path_normalize((char*)path, normalized, PBFS_MAX_PATH_LEN);

    // Root
    if (strcmp(normalized, "/") == 0) {
        return -1; // Root dmm
    }

    uint64_t current_dmm_lba = mnt->header64.dmm_root_lba;
    char part[PBFS_MAX_NAME_LEN];
    int depth = 0;
    int found = 0;

    while (depth < PBFS_MAX_DMM_CHAIN) {
        path_part(normalized, depth++, part, PBFS_MAX_NAME_LEN);
        if (strlen(part) == 0) break;

        found = 0;
        uint64_t iter_lba = current_dmm_lba;
        while (iter_lba > 0) {
            PBFS_DMM dmm;
            int out_res = pbfs_read(mnt, iter_lba, sizeof(PBFS_DMM), &dmm);
            if (out_res != PBFS_RES_SUCCESS) return out_res;

            for (uint32_t i = 0; i < dmm.entry_count; i++) {
                if (strcmp(dmm.entries[i].name, part) == 0) {
                    *out = dmm.entries[i];
                    *out_lba = iter_lba;
                    found = 1;
                    break;
                }
            }
            if (found) break;
            iter_lba = uint128_to_u64(dmm.extender_lba);
        }

        if (!found) return PBFS_ERR_File_Not_Found;

        // If we have more parts to go, this must be a directory
        char next_part[PBFS_MAX_NAME_LEN];
        path_part(normalized, depth, next_part, PBFS_MAX_NAME_LEN);
        if (strlen(next_part) > 0) {
            if (!(out->type & METADATA_FLAG_DIR)) return PBFS_ERR_Invalid_Path;
            PBFS_Metadata md = {0};
            int out_res = pbfs_read(mnt, uint128_to_u64(out->lba), sizeof(PBFS_Metadata), &md);
            if (out_res != PBFS_RES_SUCCESS) return out_res;
            if (md.data_offset < 1) {
                return PBFS_ERR_Invalid_File_Or_Directory;
            }

            current_dmm_lba = uint128_to_u64(out->lba) + md.data_offset;
        }
    }
    return PBFS_RES_SUCCESS;
}

static int add_kernel_entry(struct pbfs_mount* mnt, PBFS_Kernel_Table* cur_kt, uint64_t cur_kt_lba, uint64_t lba, uint64_t blocks, char* name) {
    PBFS_Kernel_Table kt = *cur_kt;
    PBFS_Kernel_Table64 kt64 = {0};
    kernel_table_to_kernel_table64(&kt, &kt64);
    PBFS_Kernel_Table* current = &kt;
    PBFS_Kernel_Table64* current64 = &kt64;
    uint64_t cur_lba = cur_kt_lba;

    for (int depth = 0; depth < PBFS_MAX_KERNEL_CHAIN; depth++) {
        // If there is space in current KT, add the entry
        if (current->entry_count < PBFS_KERNEL_TABLE_ENTRIES) {
            PBFS_Kernel_Entry* entry = &current->entries[current->entry_count];
            entry->lba = uint128_from_u64(lba);
            entry->count = uint128_from_u64(blocks);
            strncpy(entry->name, name, sizeof(entry->name) - 1);
            entry->name[sizeof(entry->name) - 1] = '\0';
            current->entry_count++;

            return pbfs_write(mnt, cur_lba, sizeof(PBFS_Kernel_Table), current);
        }

        // If current KT is full, check for extender
        if (current64->extender_lba > 0) {
            cur_lba = current64->extender_lba;
            int out = pbfs_read(mnt, cur_lba, sizeof(PBFS_Kernel_Table), &kt);
            if (out != PBFS_RES_SUCCESS) return out;
            continue;
        }

        // Need to allocate a new extender block
        PBFS_Kernel_Table new_extender = {0};
        new_extender.entry_count = 1;
        PBFS_Kernel_Entry* entry = &new_extender.entries[0];
        entry->lba = uint128_from_u64(lba);
        entry->count = uint128_from_u64(blocks);
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';

        uint64_t new_ext_lba = bitmap_find_blocks(&mnt->root_bitmap64, mnt->header64.data_start_lba, 1, mnt);
        if (new_ext_lba == 0) return PBFS_ERR_No_Space_Left;

        // Write new extender to disk
        int out = pbfs_write(mnt, new_ext_lba, sizeof(PBFS_Kernel_Table), &new_extender);
        if (out != PBFS_RES_SUCCESS) return out;
        bitmap_set(&mnt->root_bitmap, new_ext_lba, mnt->header64.bitmap_lba, 0, mnt, 1);

        // Link current DMM to extender
        current->extender_lba = uint128_from_u64(new_ext_lba);

        out = pbfs_write(mnt, cur_lba, sizeof(PBFS_Kernel_Table), &current);
        if (out != PBFS_RES_SUCCESS) return out;

        return PBFS_RES_SUCCESS;
    }

    return PBFS_ERR_No_Space_Left;
}

static int remove_kernel_entry(struct pbfs_mount* mnt, char* name_, uint64_t cur_kt_lba) {
    PBFS_Kernel_Table kt = {0};

    uint64_t current_lba = cur_kt_lba;
    char name[PBFS_MAX_NAME_LEN];
    path_basename(name_, name, PBFS_MAX_NAME_LEN);

    for (int depth = 0; depth < PBFS_MAX_KERNEL_CHAIN; depth++) {
        int out = pbfs_read(mnt, current_lba, sizeof(PBFS_Kernel_Table), &kt);
        if (out != PBFS_RES_SUCCESS) return out;

        int found_idx = -1;
        for (uint32_t i = 0; i < kt.entry_count; i++) {
            if (strcmp(kt.entries[i].name, name) == 0) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            // Shift entries left to fill the gap
            for (uint32_t i = found_idx; i < kt.entry_count - 1; i++) {
                kt.entries[i] = kt.entries[i + 1];
            }
            kt.entry_count--;

            // Write updated KT block
            return pbfs_write(mnt, current_lba, sizeof(PBFS_Kernel_Table), &kt);
        }

        if (UINT128_GT(kt.extender_lba, UINT128_ZERO)) {
            current_lba = uint128_to_u64(kt.extender_lba);
        } else {
            break;
        }
    }

    return PBFS_ERR_File_Not_Found;
}

static int find_kernel_entry(const char* path, PBFS_Kernel_Entry* out, uint64_t* out_lba, struct pbfs_mount* mnt) {
    char normalized[PBFS_MAX_PATH_LEN];
    path_normalize((char*)path, normalized, PBFS_MAX_PATH_LEN);

    // Root
    if (strcmp(normalized, "/") == 0) {
        return -1; // Root dmm
    }

    uint64_t current_kt_lba = mnt->header64.kernel_table_lba;
    char part[PBFS_MAX_NAME_LEN];
    int depth = 0;
    int found = 0;

    while (depth < PBFS_MAX_KERNEL_CHAIN) {
        path_part(normalized, depth++, part, PBFS_MAX_NAME_LEN);
        if (strlen(part) == 0) break;

        found = 0;
        uint64_t iter_lba = current_kt_lba;
        while (iter_lba > 0) {
            PBFS_Kernel_Table kt;
            int out_res = pbfs_read(mnt, iter_lba, sizeof(PBFS_Kernel_Table), &kt);
            if (out_res != PBFS_RES_SUCCESS) return out_res;

            for (uint32_t i = 0; i < kt.entry_count; i++) {
                if (strcmp(kt.entries[i].name, part) == 0) {
                    *out = kt.entries[i];
                    *out_lba = iter_lba;
                    found = 1;
                    break;
                }
            }
            if (found) break;
            iter_lba = uint128_to_u64(kt.extender_lba);
        }

        if (!found) return PBFS_ERR_File_Not_Found;
    }
    return PBFS_RES_SUCCESS;
}

static int retrieve_dir_dmm(int find_res, PBFS_DMM_Entry* dmme, PBFS_DMM* out, uint64_t* out_lba, struct pbfs_mount* mnt) {
    if (find_res != -1 && find_res != PBFS_RES_SUCCESS) return find_res;
    if (find_res == -1) {
        int res = pbfs_read(mnt, mnt->header64.dmm_root_lba, sizeof(PBFS_DMM), out);
        if (res != PBFS_RES_SUCCESS) return res;
        *out_lba = mnt->header64.dmm_root_lba;
        return PBFS_RES_SUCCESS;
    } else {
        PBFS_Metadata md = {0};
        uint64_t dmme_lba = uint128_to_u64(dmme->lba);
        int res = pbfs_read(mnt, dmme_lba, sizeof(PBFS_Metadata), &md);
        if (res != PBFS_RES_SUCCESS) return res;

        if (md.data_offset < 1) return PBFS_ERR_Invalid_File_Or_Directory;
        *out_lba = md.data_offset + dmme_lba;

        return pbfs_read(mnt, *out_lba, sizeof(PBFS_DMM), out);
    }
    return PBFS_ERR_UNKNOWN;
}

static void write_le32(unsigned char *buf, uint32_t x) {
    buf[0] = x & 0xFF;
    buf[1] = (x >> 8) & 0xFF;
    buf[2] = (x >> 16) & 0xFF;
    buf[3] = (x >> 24) & 0xFF;
}

static int is_little_endian(void) {
    uint32_t i = 1;
    return *((char*)&i);
}

// API
int pbfs_init(struct pbfs_funcs* functions) {
    funcs = *functions;
    return PBFS_RES_SUCCESS;
}

int pbfs_format(struct block_device* dev, uint8_t reserve_kernel_table, uint64_t boot_part_lba, uint8_t boot_part_size, uint64_t volume_id) {
    uint64_t bitmap_lba = 1 + PBFS_HDR_START_LBA;
    uint64_t dmm_lba = 2 + PBFS_HDR_START_LBA;
    uint64_t sysinfo_lba = 3 + PBFS_HDR_START_LBA;
    uint64_t data_lba = 4 + PBFS_HDR_START_LBA;
    uint64_t kernel_table_lba = 0;

    if (reserve_kernel_table == 1) {
        kernel_table_lba = 1 + PBFS_HDR_START_LBA;
        bitmap_lba++;
        dmm_lba++;
        sysinfo_lba++;
        data_lba++;
    }

    if (
        boot_part_size > 0 && (
            boot_part_lba == bitmap_lba ||
            boot_part_lba == dmm_lba ||
            boot_part_lba == sysinfo_lba ||
            boot_part_lba == data_lba ||
            boot_part_lba == kernel_table_lba ||
            boot_part_lba == PBFS_HDR_START_LBA
        )
    ) return PBFS_ERR_Argument_Invalid;

    uint8_t* disk = (uint8_t*)funcs.malloc(dev->block_size);
    if (!disk) return PBFS_ERR_Allocation_Failed;
    memset(disk, 0, dev->block_size);
    dev->write_block(dev, 0, disk);

    if (reserve_kernel_table == 1) {
        dev->write_block(dev, kernel_table_lba, disk);
    }
    if (boot_part_size > 0) {
        for (uint64_t i = boot_part_lba; i < boot_part_size; i++) {
            dev->write_block(dev, i, disk);
        }
    }
    dev->write_block(dev, sysinfo_lba, disk);
    dev->write_block(dev, dmm_lba, disk);

    bitmap_bit_set(disk, PBFS_HDR_START_LBA);
    bitmap_bit_set(disk, sysinfo_lba);
    bitmap_bit_set(disk, dmm_lba);
    bitmap_bit_set(disk, bitmap_lba);
    if (reserve_kernel_table == 1) bitmap_bit_set(disk, kernel_table_lba);

    dev->write_block(dev, bitmap_lba, disk);

    PBFS_Header* hdr = (PBFS_Header*)disk;
    hdr->block_size = dev->block_size;
    hdr->total_blocks = uint128_from_u64(dev->block_count);
    memcpy(&hdr->disk_name, dev->name, PBFS_DISK_NAME_LEN);
    memcpy(&hdr->magic, PBFS_MAGIC, PBFS_MAGIC_LEN);
    hdr->volume_id = uint128_from_u64(volume_id);
    hdr->bitmap_lba = bitmap_lba;
    hdr->kernel_table_lba = kernel_table_lba;
    hdr->dmm_root_lba = dmm_lba;
    hdr->sysinfo_lba = sysinfo_lba;
    hdr->data_start_lba = data_lba;
    if (boot_part_size > 0) {
        hdr->boot_partition_lba = boot_part_lba;
        hdr->boot_partition_size = boot_part_size;
    }

    dev->write_block(dev, PBFS_HDR_START_LBA, disk);
    dev->flush(dev);

    funcs.free(disk);

    return PBFS_RES_SUCCESS;
}

int pbfs_mount(struct block_device* dev, struct pbfs_mount* mnt) {
    // Validate info
    if (dev->block_count < 10) return PBFS_ERR_Device_Capacity_Too_Small;
    if (dev->block_size < 512) return PBFS_ERR_Device_Block_Size_Too_Small;
    dev->read_block(dev, PBFS_HDR_START_LBA, &mnt->header);

    if (!validate_hdr_magic(&mnt->header)) return PBFS_ERR_Invalid_Header;
    if (mnt->header.block_size != dev->block_size) return PBFS_ERR_Header_Unaligned;

    // Convert Header to 64-bit to save resources
    hdr_to_hdr64(&mnt->header, &mnt->header64);
    if (mnt->header64.total_blocks > dev->block_count) return PBFS_ERR_Header_Unaligned;

    if (
        mnt->header64.bitmap_lba <= 1 ||
        mnt->header64.dmm_root_lba <= 1 ||
        mnt->header64.data_start_lba <= 1 ||
        mnt->header64.sysinfo_lba <= 1
    ) return PBFS_ERR_Invalid_Header;

    // Get Bitmap/DMM/Kernel Table (if present)
    dev->read_block(dev, mnt->header64.bitmap_lba, &mnt->root_bitmap);
    dev->read_block(dev, mnt->header64.dmm_root_lba, &mnt->root_dmm);
    if (mnt->header64.kernel_table_lba > 1) {
        dev->read_block(dev, mnt->header64.kernel_table_lba, &mnt->root_kernel_table);
        mnt->kernel_table_present = true;
    }

    // Convert these too
    bitmap_to_bitmap64(&mnt->root_bitmap, &mnt->root_bitmap64);
    dmm_to_dmm64(&mnt->root_dmm, &mnt->root_dmm64);
    if (mnt->kernel_table_present)
        kernel_table_to_kernel_table64(&mnt->root_kernel_table, &mnt->root_kernel_table64);

    // Done!
    mnt->dev = dev;
    mnt->active = true;
    mnt->partition_start_lba = 0; // No parition support currently, will add soon
    
    return PBFS_RES_SUCCESS;
}

int pbfs_read_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;

    uint64_t lba = mnt->partition_start_lba + fs_block;
    mnt->dev->read_block(mnt->dev, lba, buffer);
    return PBFS_RES_SUCCESS;
}

int pbfs_write_block(struct pbfs_mount* mnt, uint64_t fs_block, void* buffer) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;

    uint64_t lba = mnt->partition_start_lba + fs_block;
    mnt->dev->write_block(mnt->dev, lba, buffer);
    return PBFS_RES_SUCCESS;
}

int pbfs_read(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;

    uint8_t buf[mnt->header64.block_size];
    uint64_t required_blocks = ALIGN_UP(size, mnt->header64.block_size) / mnt->header64.block_size;
    size_t total_read_size = 0;

    for (uint64_t i = 0; i < required_blocks; i++) {
        uint64_t lba = mnt->partition_start_lba + fs_block + i;
        mnt->dev->read_block(mnt->dev, lba, buf);

        if (total_read_size + mnt->header64.block_size > size) {
            uint64_t remaining_size = size - total_read_size;
            memcpy(buffer + total_read_size, buf, remaining_size);
            break;
        } else {
            memcpy(buffer + total_read_size, buf, mnt->header64.block_size);
            total_read_size += mnt->header64.block_size;
        }
    }
    return PBFS_RES_SUCCESS;
}

int pbfs_write(struct pbfs_mount* mnt, uint64_t fs_block, size_t size, void* buffer) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;

    uint8_t buf[mnt->header64.block_size];
    uint64_t required_blocks = ALIGN_UP(size, mnt->header64.block_size) / mnt->header64.block_size;

    uint64_t total_write_size = 0;
    for (uint64_t i = 0; i < required_blocks; i++) {
        uint64_t lba = mnt->partition_start_lba + fs_block + i;

        if (total_write_size + mnt->header64.block_size > size) {
            uint64_t remaining_size = size - total_write_size;
            memcpy(buf, buffer + total_write_size, remaining_size);
            mnt->dev->write_block(mnt->dev, lba, buf);
            break;
        } else {
            memcpy(buf, buffer + total_write_size, mnt->header64.block_size);
            total_write_size += mnt->header64.block_size;
        }

        mnt->dev->write_block(mnt->dev, lba, buf);
    }

    return PBFS_RES_SUCCESS;
}

int pbfs_flush(struct pbfs_mount* mnt) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;

    mnt->dev->flush(mnt->dev);
    return PBFS_RES_SUCCESS;
}

int pbfs_add(struct pbfs_mount* mnt, char* path, uint32_t uid, uint32_t gid, PBFS_Metadata_Flags type, PBFS_Permission_Flags permissions, uint8_t* data, size_t data_size) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;
    if (type == METADATA_FLAG_INVALID) return PBFS_ERR_Wrong_Type;
    if (permissions == PERM_INVALID) return PBFS_ERR_Wrong_Permissions;
    if (data_size < 1) return PBFS_ERR_Argument_Invalid;
    if (data == NULL) return PBFS_ERR_Argument_Invalid;

    PBFS_DMM_Entry tmp = {0};
    uint64_t tmp_lba = 0;
    if (find_dmm_entry(path, &tmp, &tmp_lba, mnt) == PBFS_RES_SUCCESS) return PBFS_ERR_File_Already_Exists;

    uint64_t required_blocks = ALIGN_UP(data_size, mnt->header64.block_size) / mnt->header64.block_size;
    
    uint64_t lba = bitmap_find_blocks(&mnt->root_bitmap64, mnt->header64.data_start_lba, required_blocks + 1, mnt);
    if (lba <= 1) return PBFS_ERR_No_Space_Left;

    char dname[PBFS_MAX_PATH_LEN];
    char name[PBFS_MAX_NAME_LEN];
    path_basename(path, name, PBFS_MAX_NAME_LEN);
    path_dirname(path, dname, PBFS_MAX_PATH_LEN);

    PBFS_DMM_Entry parent = {0};
    uint64_t parent_lba = {0};
    int out = find_dmm_entry(dname, &parent, &parent_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;

    if (out != -1) {
        // check perms
        if (!(parent.perms & PERM_WRITE)) return PBFS_ERR_Wrong_Permissions;
    }

    PBFS_DMM parent_dmm = {0};
    uint64_t parent_dmm_lba = {0};
    out = retrieve_dir_dmm(out, &parent, &parent_dmm, &parent_dmm_lba, mnt);
    if (out != PBFS_RES_SUCCESS) return out;

    PBFS_Metadata md = {0};
    memcpy(&md.name, name, strlen(name));
    md.uid = uid;
    md.gid = gid;
    md.permission_offset = 0;
    md.created_timestamp = 0;
    md.modified_timestamp = 0;
    md.data_size = uint128_from_u64(data_size);
    md.data_offset = 1;
    md.flags = type;
    md.ex_flags = permissions;
    md.extender_lba = UINT128_ZERO;

    out = pbfs_write(mnt, lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;

    out = pbfs_write(mnt, lba + 1, data_size, data);
    if (out != PBFS_RES_SUCCESS) return out;

    for (uint64_t i = 0; i < required_blocks + 1; i++) {
        bitmap_set(&mnt->root_bitmap, lba + i, mnt->header64.bitmap_lba, 0, mnt, 1);
    }

    return add_dmm_entry(mnt, &parent_dmm, parent_dmm_lba, lba, name, type, permissions, md.created_timestamp, md.modified_timestamp);
}

int pbfs_add_dir(struct pbfs_mount* mnt, char* path, uint32_t uid, uint32_t gid, PBFS_Permission_Flags permissions) {
    PBFS_DMM dmm;
    memset(&dmm, 0, sizeof(PBFS_DMM));
    return pbfs_add(mnt, path, uid, gid, METADATA_FLAG_DIR, permissions, (uint8_t*)&dmm, sizeof(PBFS_DMM));
}

int pbfs_remove(struct pbfs_mount* mnt, char* path) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;

    PBFS_DMM_Entry e = {0};
    uint64_t e_lba = 0;
    int out = find_dmm_entry(path, &e, &e_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;
    if (out == -1) return PBFS_ERR_Cannot_Remove_Root;
    if (e.perms & PERM_LOCKED) return PBFS_ERR_Wrong_Permissions;

    PBFS_Metadata md = {0};
    e_lba = uint128_to_u64(e.lba);
    out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;
    uint64_t ext_lba = uint128_to_u64(md.extender_lba);
    uint64_t md_data_size = uint128_to_u64(md.data_size);

    if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
    for (uint64_t i = 0; i < (md_data_size / mnt->header64.block_size) + 1; i++) {
        bitmap_set(&mnt->root_bitmap, i + e_lba, mnt->header64.bitmap_lba, 0, mnt, 1);
    }
    
    while (ext_lba != 0) {
        e_lba = ext_lba;
        out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &md);
        if (out != PBFS_RES_SUCCESS) return out;
        ext_lba = uint128_to_u64(md.extender_lba);

        if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
        for (uint64_t i = 0; i < (md_data_size / mnt->header64.block_size) + 1; i++) {
            bitmap_set(&mnt->root_bitmap, i + e_lba, mnt->header64.bitmap_lba, 0, mnt, 1);
        }
    }

    return remove_dmm_entry(mnt, path, e_lba);
}

int pbfs_update_file(struct pbfs_mount* mnt, char* path, uint8_t* data, size_t data_size) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;
    if (data_size < 1) return PBFS_ERR_Argument_Invalid;
    if (data == NULL) return PBFS_ERR_Argument_Invalid;

    PBFS_DMM_Entry e = {0};
    uint64_t e_lba = 0;
    int out = find_dmm_entry(path, &e, &e_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;
    if (out != -1) return PBFS_ERR_Invalid_Path;
    if (!(e.perms != PERM_WRITE)) return PBFS_ERR_Wrong_Permissions;

    PBFS_Metadata og_md = {0};
    PBFS_Metadata md = {0};
    e_lba = uint128_to_u64(e.lba);
    out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &og_md);
    memcpy(&md, &og_md, sizeof(PBFS_Metadata));
    if (out != PBFS_RES_SUCCESS) return out;
    uint64_t ext_lba = uint128_to_u64(md.extender_lba);

    uint64_t md_data_size = uint128_to_u64(md.data_size);
    if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
    for (uint64_t i = 0; i < (md_data_size / mnt->header64.block_size) + 1; i++) {
        bitmap_set(&mnt->root_bitmap, i + e_lba, mnt->header64.bitmap_lba, 0, mnt, 1);
    }
    
    while (ext_lba != 0) {
        e_lba = ext_lba;
        out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &md);
        if (out != PBFS_RES_SUCCESS) return out;
        ext_lba = uint128_to_u64(md.extender_lba);
        md_data_size = uint128_to_u64(md.data_size);

        if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
        for (uint64_t i = 0; i < (md_data_size / mnt->header64.block_size) + 1; i++) {
            bitmap_set(&mnt->root_bitmap, i + e_lba, mnt->header64.bitmap_lba, 0, mnt, 1);
        }
    }

    remove_dmm_entry(mnt, path, e_lba);
    return pbfs_add(mnt, path, og_md.uid, og_md.gid, og_md.flags, og_md.ex_flags, data, data_size);
}

int pbfs_change_permissions(struct pbfs_mount* mnt, char* path, PBFS_Permission_Flags new_permissions) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;

    if (new_permissions == PERM_INVALID) return PBFS_ERR_Wrong_Permissions;

    char dname[PBFS_MAX_PATH_LEN];
    char name[PBFS_MAX_NAME_LEN];
    path_basename(path, name, PBFS_MAX_NAME_LEN);
    path_dirname(path, dname, PBFS_MAX_PATH_LEN);

    PBFS_DMM_Entry parent = {0};
    uint64_t parent_lba = {0};
    int out = find_dmm_entry(dname, &parent, &parent_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;

    PBFS_DMM parent_dmm = {0};
    uint64_t parent_dmm_lba = {0};
    out = retrieve_dir_dmm(out, &parent, &parent_dmm, &parent_dmm_lba, mnt);
    if (out != PBFS_RES_SUCCESS) return out;

    PBFS_DMM_Entry e = {0};
    uint64_t e_lba = 0;
    out = find_dmm_entry(path, &e, &e_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;
    if (out != -1) return PBFS_ERR_Invalid_Path;
    
    PBFS_Metadata md = {0};
    uint64_t md_lba = uint128_to_u64(e.lba);
    out = pbfs_read(mnt, md_lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;

    md.ex_flags = new_permissions;
    out = pbfs_write(mnt, md_lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;

    out = remove_dmm_entry(mnt, path, e_lba);
    if (out != PBFS_RES_SUCCESS) return out;

    return add_dmm_entry(mnt, &parent_dmm, parent_dmm_lba, e_lba, name, md.flags, new_permissions, md.created_timestamp, md.modified_timestamp);
}

int pbfs_read_file(struct pbfs_mount* mnt, char* path, uint8_t** data_out, size_t* data_size) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;
    if (data_size == NULL) return PBFS_ERR_Argument_Invalid;
    if (data_out == NULL) return PBFS_ERR_Argument_Invalid;

    char dname[PBFS_MAX_PATH_LEN];
    char name[PBFS_MAX_NAME_LEN];
    path_basename(path, name, PBFS_MAX_NAME_LEN);
    path_dirname(path, dname, PBFS_MAX_PATH_LEN);

    PBFS_DMM_Entry e = {0};
    uint64_t e_lba = 0;
    int out = find_dmm_entry(path, &e, &e_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;
    if (out != -1) return PBFS_ERR_Invalid_Path;
    if (!(e.perms & PERM_READ)) return PBFS_ERR_Wrong_Permissions;
    
    PBFS_Metadata md = {0};
    uint64_t md_lba = uint128_to_u64(e.lba);
    out = pbfs_read(mnt, md_lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;

    uint64_t ext_lba = uint128_to_u64(md.extender_lba);
    uint64_t ext_lba2 = ext_lba;
    uint64_t md_data_size = uint128_to_u64(md.data_size);

    if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
    
    uint64_t total_size = md_data_size;
    while (ext_lba2 != 0) {
        e_lba = ext_lba;
        out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &md);
        if (out != PBFS_RES_SUCCESS) return out;
        ext_lba = uint128_to_u64(md.extender_lba);
        md_data_size = uint128_to_u64(md.data_size);

        if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
        total_size += md_data_size;
    }
    
    uint8_t* data = funcs.malloc(total_size);
    if (data == NULL) return PBFS_ERR_Allocation_Failed;
    *data_size = total_size;
    *data_out = data;

    out = pbfs_read(mnt, e_lba, md_data_size, data);
    if (out != PBFS_RES_SUCCESS) return out;
    
    while (ext_lba != 0) {
        e_lba = ext_lba;
        out = pbfs_read(mnt, e_lba, sizeof(PBFS_Metadata), &md);
        if (out != PBFS_RES_SUCCESS) return out;
        ext_lba = uint128_to_u64(md.extender_lba);
        md_data_size = uint128_to_u64(md.data_size);

        if (md.data_offset < 1 && md_data_size > 0) return PBFS_ERR_Invalid_File_Or_Directory;
        out = pbfs_read(mnt, e_lba, md_data_size, data);
        if (out != PBFS_RES_SUCCESS) return out;
    }

    return PBFS_RES_SUCCESS;
}

int pbfs_copy(struct pbfs_mount* mnt, char* path, char* new_path) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(path) < 1) return PBFS_ERR_No_Path;
    if (strlen(new_path) < 1) return PBFS_ERR_No_Path;

    PBFS_DMM_Entry e = {0};
    uint64_t e_lba = 0;
    int out = find_dmm_entry(path, &e, &e_lba, mnt);
    if (out != -1 && out != PBFS_RES_SUCCESS) return out;
    if (out != -1) return PBFS_ERR_Invalid_Path;

    PBFS_Metadata md = {0};
    uint64_t md_lba = uint128_to_u64(e.lba);
    out = pbfs_read(mnt, md_lba, sizeof(PBFS_Metadata), &md);
    if (out != PBFS_RES_SUCCESS) return out;

    uint8_t* data = NULL;
    size_t data_size = 0;
    out = pbfs_read_file(mnt, path, &data, &data_size);
    if (out != PBFS_RES_SUCCESS) {
        if (data_size > 0 || data != NULL) funcs.free(data);
        return out;
    }

    out = pbfs_add(mnt, new_path, md.uid, md.gid, md.flags, md.ex_flags, data, data_size);
    funcs.free(data);
    return out;
}

int pbfs_move(struct pbfs_mount* mnt, char* path, char* new_path) {
    int out = pbfs_copy(mnt, path, new_path);
    if (out != PBFS_RES_SUCCESS) return out;
    return pbfs_remove(mnt, path);
}

int pbfs_rename(struct pbfs_mount* mnt, char* path, char* new_path) {
    return pbfs_move(mnt, path, new_path);
}

int pbfs_find_entry(const char* path, PBFS_DMM_Entry* out, uint64_t* out_lba, struct pbfs_mount* mnt) {
    if (mnt->active != 1) return PBFS_ERR_Mount_Inactive;
    return find_dmm_entry(path, out, out_lba, mnt);
}

int pbfs_add_kernel(struct pbfs_mount* mnt, const char* name, uint8_t* data, size_t data_size) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(name) < 1) return PBFS_ERR_Invalid_Path;
    if (data_size < 1) return PBFS_ERR_Argument_Invalid;
    if (data == NULL) return PBFS_ERR_Argument_Invalid;

    PBFS_Kernel_Entry tmp = {0};
    uint64_t tmp_lba = 0;
    if (find_kernel_entry(name, &tmp, &tmp_lba, mnt) == PBFS_RES_SUCCESS) return PBFS_ERR_File_Already_Exists;

    uint64_t required_blocks = ALIGN_UP(data_size, mnt->header64.block_size) / mnt->header64.block_size;
    uint64_t blocks = bitmap_find_blocks(&mnt->root_bitmap64, mnt->header64.bitmap_lba, required_blocks, mnt);
    if (blocks <= 1) return PBFS_ERR_No_Space_Left;

    int out = pbfs_write(mnt, blocks, data_size, data);
    if (out != PBFS_RES_SUCCESS) return out;

    out = add_kernel_entry(mnt, &mnt->root_kernel_table, mnt->header64.kernel_table_lba, blocks, required_blocks, name);
    if (out != PBFS_RES_SUCCESS) return out;

    for (uint64_t i = blocks; i < blocks + required_blocks; i++) {
        bitmap_set(&mnt->root_bitmap, i, mnt->header64.bitmap_lba, 0, mnt, 1);
    }
    return PBFS_RES_SUCCESS;
}

int pbfs_find_kernel(struct pbfs_mount* mnt, const char* name, PBFS_Kernel_Entry* kernel_e) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(name) < 1) return PBFS_ERR_Invalid_Path;
    if (kernel_e == NULL) return PBFS_ERR_Argument_Invalid;

    uint64_t out_lba = 0;
    return find_kernel_entry(name, kernel_e, &out_lba, mnt);
}

int pbfs_get_kernel(struct pbfs_mount* mnt, PBFS_Kernel_Entry* kernel_e, uint8_t** data, size_t* data_size) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (data == NULL) return PBFS_ERR_Argument_Invalid;
    if (data_size == NULL) return PBFS_ERR_Argument_Invalid;
    if (kernel_e == NULL) return PBFS_ERR_Argument_Invalid;

    uint64_t count = uint128_to_u64(kernel_e->count);
    uint64_t lba = uint128_to_u64(kernel_e->lba);
    uint8_t* data_ret = (uint8_t*)funcs.malloc(count * mnt->header64.block_size);
    if (!data_ret) return PBFS_ERR_Allocation_Failed;

    *data_size = count * mnt->header64.block_size;
    *data = data_ret;

    return pbfs_read(mnt, lba, count * mnt->header64.block_size, data_ret);
}

int pbfs_remove_kernel(struct pbfs_mount* mnt, char* name) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (strlen(name) < 1) return PBFS_ERR_No_Path;

    PBFS_Kernel_Entry out_e = {0};
    PBFS_Kernel_Entry64 e = {0};
    uint64_t out_lba = 0;
    int out = find_kernel_entry(name, &out_e, &out_lba, mnt);
    if (out != PBFS_RES_SUCCESS) return out;
    kernelentry_to_kernelentry64(&out_e, &e);

    out = remove_kernel_entry(mnt, name, mnt->header64.kernel_table_lba);
    if (out != PBFS_RES_SUCCESS) return out;

    for (uint64_t i = e.lba; i < e.lba + e.count; i++) {
        bitmap_set(&mnt->root_bitmap, i, mnt->header64.bitmap_lba, 0, mnt, 0);
    }
    return PBFS_RES_SUCCESS;
}

int pbfs_list_kernels(struct pbfs_mount* mnt, PBFS_Kernel_Entry* out, size_t max_out_len, size_t* out_len) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (out == NULL) return PBFS_ERR_Argument_Invalid;
    if (max_out_len < 1) return PBFS_RES_SUCCESS;

    PBFS_Kernel_Table kt = {0};
    int ret = pbfs_read(mnt, mnt->header64.kernel_table_lba, sizeof(PBFS_Kernel_Table), &kt);
    if (ret != PBFS_RES_SUCCESS) return ret;
    if (kt.entry_count > PBFS_KERNEL_TABLE_ENTRIES) return PBFS_ERR_Kernel_Table_Corrupted;

    uint64_t written_entries = 0;
    uint64_t ext = 1;
    while (ext > 0) {
        size_t entry_count = (max_out_len - written_entries) > kt.entry_count ? kt.entry_count : (max_out_len - written_entries);
        for (size_t i = 0; i < entry_count; i++) {
            if (written_entries >= max_out_len) {*out_len = written_entries; return PBFS_RES_SUCCESS;}
            out[written_entries++] = kt.entries[i];
        }
        ext = uint128_to_u64(kt.extender_lba);
        if (ext > 0) {
            ret = pbfs_read(mnt, ext, sizeof(PBFS_Kernel_Table), &kt);
            if (ret != PBFS_RES_SUCCESS) return ret;
            if (kt.entry_count > PBFS_KERNEL_TABLE_ENTRIES) return PBFS_ERR_Kernel_Table_Corrupted;
        }
    }
    *out_len = written_entries;
    return PBFS_RES_SUCCESS;
}

int pbfs_add_bootloader(struct pbfs_mount *mnt, uint8_t *data, size_t data_size, uint8_t boot_part_type) {
    if (!mnt->active) return PBFS_ERR_Mount_Inactive;
    if (data == NULL || data_size < 1) return PBFS_ERR_Argument_Invalid;

    if (mnt->header64.boot_partition_size < 1) return PBFS_ERR_No_Bootloader_Partition;
    if (mnt->header64.boot_partition_size < 2) return PBFS_ERR_Bootloader_Partition_Too_Small;

    if (boot_part_type == PBFS_BOOT_PART_TYPE_NONE) {
        char tmp[512];
        memcpy(tmp, data, data_size > 512 ? 512 : data_size);
        // Just write data to sector 0
        return pbfs_write(mnt, 0, 512, tmp);
    } else if (boot_part_type == PBFS_BOOT_PART_TYPE_MBR) {
        struct btl_mbr mbr = {0};
        memcpy(mbr.bootstrap_code, data, data_size > 446 ? 446 : data_size);

        struct btl_mbr_parition_entry* p0 = &mbr.partition_table[0];
        p0->bootable_flag = 0x80; // Active/Bootable
        p0->partition_type = 0x0;
        p0->start_lba = mnt->header64.boot_partition_lba;
        p0->size_sectors = (uint32_t)mnt->header64.boot_partition_size;

        mbr.mbr_signature = 0xAA55;

        return pbfs_write(mnt, 0, sizeof(struct btl_mbr), &mbr);
    } else if (boot_part_type == PBFS_BOOT_PART_TYPE_GPT) {
        struct btl_mbr pmbr = {0};
        
        struct btl_mbr_parition_entry* pm0 = &pmbr.partition_table[0];
        pm0->bootable_flag = 0x00;
        pm0->partition_type = 0xEE;
        uint32_t tmp = 0x000200;
        memcpy(&pm0->start_chs, &tmp, 3);
        tmp = (mnt->dev->block_count - 1) > 0xFFFFFF ? 0xFFFFFF : (mnt->dev->block_count - 1);
        memcpy(&pm0->end_chs, &tmp, 3);
        pm0->start_lba = 0x00000001;
        tmp = (mnt->dev->block_count - 1) > 0xFFFFFFFF ? 0xFFFFFFFF : (mnt->dev->block_count - 1);
        pm0->size_sectors = tmp;
        
        int out = pbfs_write(mnt, 0, 512, &pmbr);
        if (out != PBFS_RES_SUCCESS) return out;

        struct btl_gpt_hdr gpt_hdr = {0};
        memcpy(gpt_hdr.signature, "EFI PART", 8);
        gpt_hdr.revision = 0x00010000; // 1.0
        gpt_hdr.header_size = sizeof(struct btl_gpt_hdr);
        gpt_hdr.mylba = 1;
        gpt_hdr.alternate_lba = mnt->dev->block_count - 1; // Last sector
        gpt_hdr.first_usable_lba = 34; // After Header (1) + Array (32)
        gpt_hdr.last_usable_lba = mnt->dev->block_count - 34;

        gpt_hdr.partition_entry_lba = 2;
        gpt_hdr.number_of_partition_entries = 128;
        gpt_hdr.size_of_partition_entries = 128;

        struct btl_gpt_entry* entries = funcs.malloc(sizeof(struct btl_gpt_entry) * 128);
        if (!entries) return PBFS_ERR_Allocation_Failed;

        const uint8_t efi_system_part_guid[16] = { 0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B };
        memcpy(entries[0].partition_type_guid, efi_system_part_guid, 16);
        generate_guid(entries[0].unique_partition_guid);

        entries[0].starting_lba = mnt->header64.boot_partition_lba;
        entries[0].ending_lba = (entries[0].starting_lba + mnt->header64.boot_partition_size) - 1;

        entries[0].attributes = 1;
        uint16_t efi_part_name[36] = u"EFI System Partition";
        memcpy(entries[0].partition_name, efi_part_name, sizeof(efi_part_name));

        if (crc32_initialized != 1) crc32_init();
        gpt_hdr.partition_entry_array_crc32 = crc32(entries, 128 * sizeof(struct btl_gpt_entry));
        
        generate_guid(gpt_hdr.disk_guid);
        
        gpt_hdr.header_crc32 = 0;
        gpt_hdr.header_crc32 = crc32(&gpt_hdr, sizeof(struct btl_gpt_hdr));

        out = pbfs_write(mnt, 1, 512, &gpt_hdr);
        if (out != PBFS_RES_SUCCESS) { funcs.free(entries); return out; }
        out = pbfs_write(mnt, 2, 128 * sizeof(struct btl_gpt_entry), entries);
        if (out != PBFS_RES_SUCCESS) { funcs.free(entries); return out; }

        struct btl_gpt_hdr backup_hdr = gpt_hdr;
        backup_hdr.mylba = mnt->dev->block_count - 1;
        backup_hdr.alternate_lba = 1;
        backup_hdr.partition_entry_lba = mnt->dev->block_count - 33;
        
        backup_hdr.header_crc32 = 0;
        backup_hdr.header_crc32 = crc32(&backup_hdr, sizeof(struct btl_gpt_hdr));

        out = pbfs_write(mnt, backup_hdr.mylba, 512, &backup_hdr);
        if (out != PBFS_RES_SUCCESS) { funcs.free(entries); return out; }

        out = pbfs_write(mnt, backup_hdr.partition_entry_lba, 128 * sizeof(struct btl_gpt_entry), entries);
        if (out != PBFS_RES_SUCCESS) { funcs.free(entries); return out; }

        out = pbfs_write(mnt, entries[0].starting_lba, data_size, data);
        funcs.free(entries);
        return out;
    } else {
        return PBFS_ERR_Argument_Invalid;
    }
}