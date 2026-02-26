#define CLI
#include <pbfs.h>
#include <pbfs-cli.h>
#include <pbfs_structs.h>
#undef CLI

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define LBA(lba, block_size) lba * block_size

static size_t align_up(size_t value, size_t alignment) {
    if (alignment == 0) return value;
    size_t remainder = value % alignment;
    return remainder == 0 ? value : value + alignment - remainder;
}

// Util Funcs for bitmap
static int bitmap_test_recursive(PBFS_Bitmap* current, uint128_t lba, uint128_t bitmap_idx, FILE* fp, int block_size) {
    if (is_lba_in_current_bitmap(lba, bitmap_idx)) {
        uint64_t local_bit = uint128_to_u64(lba) % (PBFS_BITMAP_LIMIT * 8);
        return bitmap_bit_test(current->bytes, local_bit);
    }

    uint64_t ext_lba = uint128_to_u64(current->extender_lba); // Cur-Technology supports 64-bit addressing only, 128-bit addressing is for future-proofing
    if (ext_lba <= 1) return -1;

    PBFS_Bitmap next = {0};
    fseek(fp, LBA(ext_lba, block_size), SEEK_SET);
    if (fread(&next, sizeof(PBFS_Bitmap), 1, fp) == 0) return -1;
    uint128_t bitmap_idx_inced = bitmap_idx;
    uint128_inc(&bitmap_idx_inced);
    return bitmap_test_recursive(&next, lba, bitmap_idx_inced, fp, block_size);
}
static uint128_t bitmap_find_blocks(PBFS_Bitmap* start, uint128_t blocks, FILE* fp, int block_size) {
    if (uint128_is_zero(&blocks))
        return UINT128_ZERO;
    
    PBFS_Bitmap current = *start;
    uint128_t bitmap_idx = UINT128_ZERO;

    uint128_t run_start = UINT128_ZERO;
    uint128_t run_length = UINT128_ZERO;
    uint8_t in_run = 0;

    for (uint64_t depth = 0; depth < PBFS_MAX_BITMAP_CHAIN; depth++) {

        for (uint64_t i = 0; i < (PBFS_BITMAP_LIMIT * 8); i++) {

            uint128_t global_lba;
            uint128_mul_u32(&global_lba, &bitmap_idx, PBFS_BITMAP_LIMIT * 8);
            uint128_t i_128 = uint128_from_u64(i);
            uint128_add(&global_lba, &global_lba, &i_128);

            if (!bitmap_bit_test(current.bytes, i)) {
                if (!in_run) {
                    run_start = global_lba;
                    run_length = UINT128_ZERO;
                    in_run = 1;
                }
                uint128_inc(&run_length);
                if (UINT128_EQ(run_length, blocks))
                    return run_start;
            } else {
                in_run = 0;
                run_length = UINT128_ZERO;
            }
        }
        uint64_t next_ext_lba = uint128_to_u64(current.extender_lba);
        if (next_ext_lba <= 1)
            return UINT128_ZERO;

        fseek(fp, LBA(next_ext_lba, block_size), SEEK_SET);
        if (fread(&current, sizeof(PBFS_Bitmap), 1, fp) != 1)
            return UINT128_ZERO;

        uint128_inc(&bitmap_idx);
    }

    return UINT128_ZERO;
}
static uint128_t bitmap_set(PBFS_Bitmap* current, uint128_t lba, uint128_t current_lba, uint128_t bitmap_idx, FILE* fp, int block_size, uint8_t value) { // value: 1/0 (default: 0)
    if (is_lba_in_current_bitmap(lba, bitmap_idx)) {
        uint64_t local_bit = uint128_to_u64(lba) % (PBFS_BITMAP_LIMIT * 8);
        if (value == 1)
            bitmap_bit_set(current->bytes, local_bit);
        else
            bitmap_bit_clear(current->bytes, local_bit);
        fseek(fp, LBA(uint128_to_u64(current_lba), block_size), SEEK_SET);
        fwrite(current, sizeof(PBFS_Bitmap), 1, fp);
        return lba;
    }

    // Need next extender
    uint64_t next_ext_lba = uint128_to_u64(current->extender_lba);
    uint128_t next_ext = current->extender_lba;
    PBFS_Bitmap next = {0};

    if (next_ext_lba <= 1) {
        // Find a free block in CURRENT bitmap to use as the NEW extension block
        // use the first available bit in the current data range
        uint128_t new_block = UINT128_ZERO;
        uint8_t found = 0;
        for (uint64_t i = 0; i < (PBFS_BITMAP_LIMIT * 8); i++) {
            if (!bitmap_bit_test(current->bytes, i)) {
                uint128_mul_u32(&new_block, &bitmap_idx, PBFS_BITMAP_LIMIT * 8);
                uint128_t i_128 = uint128_from_u64(i);
                uint128_add(&new_block, &new_block, &i_128);
                bitmap_bit_set(current->bytes, i);
                found = 1;
                break;
            }
        }

        if (found == 0) return UINT128_ZERO; // Truly out of space

        // Link current to new
        current->extender_lba = new_block;
        fseek(fp, LBA(uint128_to_u64(current_lba), block_size), SEEK_SET);
        fwrite(current, sizeof(PBFS_Bitmap), 1, fp);

        // Initialize new bitmap block
        memset(&next, 0, sizeof(PBFS_Bitmap));
        fseek(fp, LBA(uint128_to_u64(new_block), block_size), SEEK_SET);
        fwrite(&next, sizeof(PBFS_Bitmap), 1, fp);

        // Continue recursion with the newly created block
        next_ext = new_block;
    } else {
        fseek(fp, LBA(next_ext_lba, block_size), SEEK_SET);
        if (fread(&next, sizeof(PBFS_Bitmap), 1, fp) == 0) return UINT128_ZERO;
    }

    uint128_t bitmap_idx_inced = bitmap_idx;
    uint128_inc(&bitmap_idx_inced);
    return bitmap_set(&next, lba, next_ext, bitmap_idx_inced, fp, block_size, value);
}
// Util Funcs for parsing perms and types
static PBFS_Metadata_Flags parse_file_type(char* str) {
    PBFS_Metadata_Flags type = METADATA_FLAG_INVALID;
    if (strcmp(str, "dir") == 0) {
        type = METADATA_FLAG_DIR;
    } else if (strcmp(str, "file") == 0) {
        type = METADATA_FLAG_FILE;
    } else if (strcmp(str, "res") == 0) {
        type = METADATA_FLAG_RES;
    } else if (strcmp(str, "sys") == 0) {
        type = METADATA_FLAG_SYS;
    }
    return type;
}
static PBFS_Permission_Flags parse_file_perms(char* str) {
    PBFS_Permission_Flags perms = PERM_INVALID;
    char str_c[6]; // ensures proper parsing + speed due to switch
    strncpy(str_c, str, sizeof(str_c));
    size_t slen = strlen(str_c);

    for (uint32_t i = 0; i < slen; i++) {
        char c = str_c[i];
        switch (c) {
            case 'r': perms |= PERM_READ; break;
            case 'w': perms |= PERM_WRITE; break;
            case 'e': perms |= PERM_EXEC; break;
            case 'l': perms |= PERM_LOCKED; break;
            case 'p': perms |= PERM_PROTECTED; break;
            case 's': perms |= PERM_SYS; break;
            default: perms = PERM_INVALID; return perms; // No mercy
        }
    }

    return perms;
}
static const char* file_type_to_str(PBFS_Metadata_Flags type) {
    switch (type) {
        case METADATA_FLAG_INVALID: return "Invalid";
        case METADATA_FLAG_DIR: return "Directory";
        case METADATA_FLAG_FILE: return "File";
        case METADATA_FLAG_SYS: return "System";
        case METADATA_FLAG_RES: return "Reserved";
        case METADATA_FLAG_PBFS: return "PBFS";
        default: "Invalid";
    }
}
static void file_perms_to_str(PBFS_Permission_Flags perms, char* out, size_t size) {
    if (perms == PERM_INVALID) {
        strncpy(out, "Invalid", size);
        return;
    }
    if (size < 6) return;
    size_t off = 0;
    if (perms & PERM_READ)
        out[off++] = 'r';
    if (perms & PERM_WRITE)
        out[off++] = 'w';
    if (perms & PERM_EXEC)
        out[off++] = 'e';
    if (perms & PERM_SYS)
        out[off++] = 's';
    if (perms & PERM_LOCKED)
        out[off++] = 'l';
    if (perms & PERM_PROTECTED)
        out[off++] = 'p';
    return;
}
static void pbfs_free_file_blocks(FILE* fp, PBFS_Header* hdr, PBFS_Bitmap* bitmap, uint128_t metadata_lba) {
    PBFS_Metadata meta;
    fseek(fp, LBA(uint128_to_u64(metadata_lba), hdr->block_size), SEEK_SET);
    if (fread(&meta, sizeof(PBFS_Metadata), 1, fp) != 1) return;

    // Calculate blocks: Metadata block + Data blocks
    uint64_t data_blocks = (uint128_to_u64(meta.data_size) + hdr->block_size - 1) / hdr->block_size;
    uint128_t current_lba = metadata_lba;

    // Free the metadata block and all subsequent data blocks
    for (uint64_t i = 0; i < data_blocks + 1; i++) {
        bitmap_set(bitmap, current_lba, uint128_from_u64(hdr->bitmap_lba), UINT128_ZERO, fp, hdr->block_size, 0);
        uint128_inc(&current_lba);
    }
}
static void pbfs_delete_recursive(FILE* fp, PBFS_Header* hdr, PBFS_Bitmap* bitmap, uint128_t dmm_lba) {
    PBFS_DMM dmm;
    uint128_t current_dmm_lba = dmm_lba;

    while (UINT128_GT(current_dmm_lba, UINT128_ZERO)) {
        fseek(fp, LBA(uint128_to_u64(current_dmm_lba), hdr->block_size), SEEK_SET);
        if (fread(&dmm, sizeof(PBFS_DMM), 1, fp) != 1) break;

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (dmm.entries[i].type & METADATA_FLAG_DIR) {
                // Recursively clear the subdirectory's DMM chain
                pbfs_delete_recursive(fp, hdr, bitmap, dmm.entries[i].lba);
            } else {
                // Clear the file's data blocks
                pbfs_free_file_blocks(fp, hdr, bitmap, dmm.entries[i].lba);
            }
        }

        // Save extender LBA before freeing current DMM block
        uint128_t next_dmm = dmm.extender_lba;
        
        // Free the DMM block itself in the bitmap
        bitmap_set(bitmap, current_dmm_lba, uint128_from_u64(hdr->bitmap_lba), UINT128_ZERO, fp, hdr->block_size, 0);
        
        current_dmm_lba = next_dmm;
    }
}

// Verifies the header
int pbfs_verify_header(PBFS_Header* header) {
    if (memcmp(&header->magic, PBFS_MAGIC, PBFS_MAGIC_LEN) != 0) return HeaderVerificationFailed;
    return EXIT_SUCCESS;
}

// Creates and Formats the disk
int pbfs_format(const char* image, uint8_t kernel_table, uint8_t bootpartition, uint8_t bootpart_size, uint32_t block_size, uint128_t total_blocks, char* disk_name, uint128_t volume_id) {
    if (block_size < 512) {
        fprintf(stderr, "Block Size cannot be less than 512 bytes!\n");
        return InvalidArgument;
    }
    uint128_t u128_10 = uint128_from_u32(10);
    if (UINT128_LT(total_blocks, u128_10)) {
        fprintf(stderr, "Total Blocks cannot be less than 10!\n");
        return InvalidArgument;
    }
    
    if (bootpart_size < 1) {
        bootpartition = 0;
    }

    printf(
        "Block Size: %d\nTotal Blocks: %lld\nVolume Id: %lld\nDisk Name: %s\n",
        (unsigned int)block_size, 
        (unsigned long long int)uint128_to_u64(total_blocks),
        (unsigned long long int)uint128_to_u64(volume_id), 
        disk_name
    );

    FILE* fp = fopen(image, "wb");
    if (!fp) {
        perror("Could not open file!\n");
        return FileError;
    }

    // Allocate Blocks
    uint8_t* disk = (uint8_t*)calloc(1, block_size);
    if (!disk) {
        perror("Allocation Failed!\n");
        fclose(fp);
        return AllocFailed;
    }
    uint8_t block[512] = {0};
    
    // Write MBR/GPT (512 - only!)
    fseek(fp, 0, SEEK_SET);
    fwrite(block, 512, 1, fp);
    // Set up Header
    PBFS_Header* hdr = (PBFS_Header*)disk;
    memset(hdr, 0, sizeof(PBFS_Header));

    memcpy(&hdr->magic, PBFS_MAGIC, PBFS_MAGIC_LEN);
    hdr->block_size = block_size;
    hdr->total_blocks = total_blocks;
    hdr->volume_id = volume_id;
    strncpy(hdr->disk_name, disk_name, sizeof(hdr->disk_name));
    
    uint64_t bitmap_lba = 2;
    uint64_t dmm_lba = 3;
    uint64_t data_lba = 5;
    uint64_t kernel_table_lba = 0;
    uint64_t boot_part_lba = 0;
    uint64_t sysinfo_lba = 4;

    if (kernel_table == 1) {
        kernel_table_lba = 2;
        bitmap_lba++;
        dmm_lba++;
        sysinfo_lba++;
        data_lba++;
        printf("Kernel Table: Reserved 1 Block\n");
    }
    if (bootpartition == 1) {
        boot_part_lba = data_lba;
        data_lba += bootpart_size;
        printf("Boot Partition: Reserved %d Blocks\n", bootpart_size);
    }

    hdr->bitmap_lba = bitmap_lba;
    hdr->kernel_table_lba = kernel_table_lba;
    hdr->dmm_root_lba = dmm_lba;
    hdr->sysinfo_lba = sysinfo_lba;
    hdr->boot_partition_lba = boot_part_lba;
    hdr->data_start_lba = data_lba;

    fseek(fp, 512, SEEK_SET);
    fwrite(disk, block_size, 1, fp);

    memset(disk, 0, block_size);
    if (kernel_table) {
        fseek(fp, LBA(kernel_table_lba, block_size), SEEK_SET);
        fwrite(disk, block_size, 1, fp);
    }
    fseek(fp, LBA(sysinfo_lba, block_size), SEEK_SET);
    fwrite(disk, block_size, 1, fp);
    fseek(fp, LBA(dmm_lba, block_size), SEEK_SET);
    fwrite(disk, block_size, 1, fp);
    fseek(fp, LBA(boot_part_lba, block_size), SEEK_SET);
    for (uint8_t i = 0; i < bootpart_size; i++) {
        fwrite(disk, block_size, 1, fp);
    }

    PBFS_Bitmap* bitmap = (PBFS_Bitmap*)disk;
    bitmap_bit_set((uint8_t*)&bitmap->bytes, 0);
    bitmap_bit_set((uint8_t*)&bitmap->bytes, 1);
    bitmap_bit_set((uint8_t*)&bitmap->bytes, bitmap_lba);
    bitmap_bit_set((uint8_t*)&bitmap->bytes, dmm_lba);
    bitmap_bit_set((uint8_t*)&bitmap->bytes, sysinfo_lba);
    if (bootpartition) bitmap_bit_set((uint8_t*)&bitmap->bytes, boot_part_lba);
    if (kernel_table) bitmap_bit_set((uint8_t*)&bitmap->bytes, kernel_table_lba);
    fseek(fp, LBA(bitmap_lba, block_size), SEEK_SET);
    fwrite(disk, block_size, 1, fp);

    memset(disk, 0, block_size);
    fseek(fp, LBA(data_lba, block_size), SEEK_SET);
    uint128_t total_blocks_idx = total_blocks;
    uint128_dec(&total_blocks_idx);
    for (uint128_t i = UINT128_ZERO; UINT128_NEQ(i, total_blocks_idx); uint128_inc(&i)) {
        fwrite(disk, block_size, 1, fp);
    }

    free(disk);
    fclose(fp);
    return EXIT_SUCCESS;
}

// Creates the disk only
int pbfs_create(const char* image, uint32_t block_size, uint128_t total_blocks) {
    // Creates the .img
    // Check if extension is provided in the filename though '.'
    if (strstr(image, ".") == NULL) {
        // Add the .pbfs extension
        strcat(image, ".pbfs");
    }

    FILE* fp = fopen(image, "wb");
    if (!fp) return FileError;

    // Allocate full disk
    uint64_t* disk = calloc(1, block_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }

    // Now write zeroes
    uint128_t total_blocks_idx = total_blocks;
    uint128_dec(&total_blocks_idx);
    memset(disk, 0, block_size);
    for (uint128_t i = UINT128_ZERO; UINT128_NEQ(i, total_blocks_idx); uint128_inc(&i)) {
        fwrite(disk, block_size, 1, fp);
    }

    free(disk);
    fclose(fp);
    return EXIT_SUCCESS;
}

// Adds a DMM entry to the disk
int pbfs_add_dmm_entry(FILE* image, PBFS_Header* hdr, PBFS_Bitmap* bitmap, uint128_t lba, char* name, PBFS_Metadata_Flags type, PBFS_Permission_Flags perms, uint64_t created_ts, uint64_t modified_ts) {
    if (hdr->dmm_root_lba <= 1) {
        fprintf(stderr, "Invalid DMM LBA in Header!\n");
        return InvalidHeader;
    }

    PBFS_DMM dmm = {0};
    fseek(image, LBA(hdr->dmm_root_lba, hdr->block_size), SEEK_SET);
    if (fread(&dmm, sizeof(PBFS_DMM), 1, image) != 1) {
        perror("Failed to read DMM root");
        return FileError;
    }

    PBFS_DMM* current = &dmm;
    uint128_t cur_lba = uint128_from_u64(hdr->dmm_root_lba);

    for (int depth = 0; depth < PBFS_MAX_DMM_CHAIN; depth++) {
        // If there is space in current DMM, add the entry
        if (current->entry_count < PBFS_DMM_ENTRIES) {
            PBFS_DMM_Entry* entry = &current->entries[current->entry_count];
            entry->lba = lba;
            entry->type = type;
            entry->created_timestamp = created_ts;
            entry->modified_timestamp = modified_ts;
            entry->perms = perms;
            strncpy(entry->name, name, sizeof(entry->name) - 1);
            entry->name[sizeof(entry->name) - 1] = '\0';
            current->entry_count++;

            fseek(image, LBA(uint128_to_u64(cur_lba), hdr->block_size), SEEK_SET);
            fwrite(current, sizeof(PBFS_DMM), 1, image);
            return EXIT_SUCCESS;
        }

        // If current DMM is full, check for extender
        if (UINT128_GT(current->extender_lba, UINT128_ZERO)) {
            cur_lba = current->extender_lba;
            fseek(image, LBA(uint128_to_u64(cur_lba), hdr->block_size), SEEK_SET);
            if (fread(&dmm, sizeof(PBFS_DMM), 1, image) != 1) {
                perror("Failed to read DMM extender");
                return FileError;
            }
            current = &dmm;
            continue;
        }

        // Need to allocate a new extender block
        PBFS_DMM new_extender = {0};
        new_extender.entry_count = 1;
        PBFS_DMM_Entry* entry = &new_extender.entries[0];
        entry->lba = lba;
        entry->type = type;
        entry->created_timestamp = created_ts;
        entry->modified_timestamp = modified_ts;
        entry->perms = perms;
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';

        uint128_t new_ext_lba = bitmap_find_blocks(bitmap, uint128_from_u32(1), image, hdr->block_size);
        if (uint128_is_zero(&new_ext_lba)) {
            fprintf(stderr, "Out of space for DMM extender!\n");
            return NoSpaceAvailable;
        }

        // Write new extender to disk
        fseek(image, LBA(uint128_to_u64(new_ext_lba), hdr->block_size), SEEK_SET);
        fwrite(&new_extender, sizeof(PBFS_DMM), 1, image);
        fseek(image, LBA(hdr->bitmap_lba, hdr->block_size), SEEK_SET);
        bitmap_set(bitmap, new_ext_lba, uint128_from_u64(hdr->bitmap_lba), UINT128_ZERO, image, hdr->block_size, 1);

        // Link current DMM to extender
        current->extender_lba = new_ext_lba;
        fseek(image, LBA(uint128_to_u64(cur_lba), hdr->block_size), SEEK_SET);
        fwrite(current, sizeof(PBFS_DMM), 1, image);

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "DMM chain exceeded max depth!\n");
    return NoSpaceAvailable;
}

// Adds a file to the disk EXTENDED
int pbfs_add_ex(FILE* image, uint8_t* data, size_t data_size, PBFS_Header* hdr, char* name, uint32_t uid, uint32_t gid, PBFS_Metadata_Flags type, PBFS_Permission_Flags perms) {
    // DATA should be PADDED to BLOCK SIZE!
    if (hdr->bitmap_lba <= 1) {
        fprintf(stderr, "Invalid Header!\n");
        return InvalidHeader;
    }
    uint8_t* disk = (uint8_t*)calloc(1, hdr->block_size);
    if (!disk) {
        fprintf(stderr, "Allocation Failed!\n");
        return AllocFailed;
    }
    fseek(image, LBA(hdr->bitmap_lba, hdr->block_size), SEEK_SET);
    fread(disk, hdr->block_size, 1, image);

    uint64_t data_blocks = data_size / hdr->block_size;

    PBFS_Bitmap bitmap = {0};
    fseek(image, LBA(hdr->bitmap_lba, hdr->block_size), SEEK_SET);
    fread(&bitmap, sizeof(PBFS_Bitmap), 1, image);
    uint128_t lba = bitmap_find_blocks(&bitmap, uint128_from_u64(data_blocks + 1), image, hdr->block_size);
    if (uint128_is_zero(&lba) == 1) {
        fprintf(stderr, "No Space left!\n");
        free(disk);
        return NoSpaceAvailable;
    }

    PBFS_Metadata* metadata = (PBFS_Metadata*)disk;
    memset(disk, 0, hdr->block_size); // To ensure no issues occur

    metadata->uid = uid;
    metadata->gid = gid;
    metadata->permission_offset = 0; // Inbuilt

    time_t now = time(NULL);
    metadata->created_timestamp = (uint64_t)now;
    metadata->modified_timestamp = (uint64_t)now;
    metadata->data_size = uint128_from_u64(data_size);
    metadata->data_offset = 2;
    metadata->flags = (uint32_t)type;
    metadata->ex_flags = (uint32_t)perms;
    metadata->extender_lba = UINT128_ZERO;

    fseek(image, LBA(uint128_to_u64(lba), hdr->block_size), SEEK_SET);
    fwrite(disk, hdr->block_size, 1, image);
    fwrite(data, 1, data_size, image);
    fseek(image, LBA(hdr->bitmap_lba, hdr->block_size), SEEK_SET);
    uint128_t current = lba;
    for (uint64_t i = 0; i < data_blocks + 1; i++) {
        bitmap_set(&bitmap, current, uint128_from_u64(hdr->bitmap_lba), UINT128_ZERO, image, hdr->block_size, 1);
        uint128_inc(&current);
    }

    pbfs_add_dmm_entry(image, hdr, &bitmap, lba, name, type, perms, (uint64_t)now, (uint64_t)now);

    free(disk);
    return EXIT_SUCCESS;
}

// Test and print Disk Info
int pbfs_test(const char* image, uint8_t debug) {
    PBFS_Header* hdr = (PBFS_Header*)calloc(1, sizeof(PBFS_Header));
    if (!hdr) {
        perror("Failed to allocate memory!\n");
        return AllocFailed;
    }
    FILE* f = fopen(image, "rb");
    if (!f) {
        perror("Failed to open image!\n");
        free(hdr);
        return FileError;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    if (size < 512 + sizeof(PBFS_Header)) {
        fprintf(stderr, "Invalid Disk!\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    }

    fseek(f, 512, SEEK_SET);
    fread(hdr, sizeof(PBFS_Header), 1, f);
    if (pbfs_verify_header(hdr) != EXIT_SUCCESS) {
        fprintf(stderr, "Invalid Header!\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    }

    if (debug == 1) printf(
        "=== Header Information ===\n"
        "Magic: %.6s\n"
        "Block Size: %d\nTotal Blocks: %lld\n"
        "Disk Name: %s\nVolume ID: %lld\n",
        hdr->magic,
        (unsigned int)hdr->block_size, (unsigned long long int)uint128_to_u64(hdr->total_blocks),
        (char*)hdr->disk_name, (unsigned long long int)uint128_to_u64(hdr->volume_id)
    );

    if (hdr->bitmap_lba <= 1) {
        fprintf(stderr, "Error: Bitmap LBA INVALID\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    } else if (hdr->sysinfo_lba <= 1) {
        fprintf(stderr, "Error: SystemInfo LBA INVALID\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    } else if (hdr->dmm_root_lba <= 1) {
        fprintf(stderr, "Error: DMM LBA INVALID\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    } else if (hdr->data_start_lba <= 1) {
        fprintf(stderr, "Error: Data LBA INVALID\n");
        fclose(f);
        free(hdr);
        return InvalidHeader;
    }

    uint8_t* disk = (uint8_t*)calloc(1, hdr->block_size);
    if (!disk) {
        perror("Failed to allocate memory!\n");
        fclose(f);
        free(hdr);
        return AllocFailed;
    }

    if (debug == 1) {
        if (hdr->boot_partition_lba <= 1) {
            printf("Info: No Boot Partition Found\n");
        } else {
            printf("\n=== Boot Partiton ===\nSize (In Blocks): %lld\n", (unsigned long long int)(hdr->data_start_lba - hdr->boot_partition_lba));
        } 
        if (hdr->kernel_table_lba <= 1) {
            printf("Info: No Kernel Table Found\n");
        } else {
            uint64_t kto = hdr->kernel_table_lba;
            PBFS_Kernel_Table* kernel_table = (PBFS_Kernel_Table*)disk;
            uint128_t test_lba = uint128_from_u32(1);

            printf("\n=== Kernel Table ===\n");
            while (kto > 1) {
                fseek(f, LBA(kto, hdr->block_size), SEEK_SET);
                fread(disk, hdr->block_size, 1, f);

                for (int i = 0; i < PBFS_KERNEL_TABLE_ENTRIES; i++) {
                    PBFS_Kernel_Entry* entry = &kernel_table->entries[i];
                    if (UINT128_NEQ(entry->lba, test_lba))
                        continue;
                    printf(
                        "== Kernel Entry (%d) ==\n"
                        "\tName: %.64s"
                        "\tLBA: %lld\n",
                        i, entry->name,
                        (unsigned long long int)uint128_to_u64(entry->lba)
                    );
                }

                kto = uint128_to_u64(kernel_table->extender_lba);
            }
        }
        PBFS_DMM dmm = {0};
        uint128_t cur_lba = uint128_from_u64(hdr->dmm_root_lba);

        printf("\n=== Files/Dirs/... ===\n");
        for (int depth = 0; depth < PBFS_MAX_DMM_CHAIN; depth++) {
            fseek(f, LBA(uint128_to_u64(cur_lba), hdr->block_size), SEEK_SET);
            if (fread(&dmm, sizeof(PBFS_DMM), 1, f) != 1) {
                fprintf(stderr, "Failed to read DMM block at LBA %llu\n", (unsigned long long int)uint128_to_u64(cur_lba));
                break;
            }

            for (uint32_t i = 0; i < dmm.entry_count; i++) {
                PBFS_DMM_Entry* entry = &dmm.entries[i];
                struct tm* created = localtime(&entry->created_timestamp);
                struct tm* modified = localtime(&entry->modified_timestamp);
                char perms_str[10] = {0};
                file_perms_to_str((PBFS_Permission_Flags)entry->perms, perms_str, 10);
                printf("(%d) %.64s (%s) at %lld (%.10s)\n", 
                    i + (depth * PBFS_DMM_ENTRIES),
                    entry->name,
                    file_type_to_str((PBFS_Metadata_Flags)entry->type),
                    (unsigned long long int)uint128_to_u64(entry->lba),
                    perms_str
                );
                printf(
                    "\tCreated: %04d-%02d-%02d\n",
                    created->tm_year + 1900,
                    created->tm_mon + 1,
                    created->tm_mday
                );
                printf(
                    "\tModified: %04d-%02d-%02d\n",
                    modified->tm_year + 1900,
                    modified->tm_mon + 1,
                    modified->tm_mday
                );
            }

            // Follow extender if exists
            if (UINT128_EQ(dmm.extender_lba, UINT128_ZERO))
                break;

            cur_lba = dmm.extender_lba;
        }
    }

    if (debug) printf("Success!\n");

    free(hdr);
    free(disk);
    fclose(f);
    return EXIT_SUCCESS;
}

// Adds a file to the disk
int pbfs_add(const char* image, const char* file, char* name, char* perms, char* type, uint32_t gid, uint32_t uid) {
    int out = pbfs_test(image, 0);
    if (out != EXIT_SUCCESS) {
        fprintf(stderr, "Invalid image!\n");
        return out;
    }

    FILE* f = fopen(image, "rb+");
    if (!f) {
        perror("Failed to open image!\n");
        return FileError;
    }

    fseek(f, 512, SEEK_SET);
    PBFS_Header* hdr = (PBFS_Header*)calloc(1, sizeof(PBFS_Header));
    if (!hdr) {
        perror("Failed to Allocate Memory!\n");
        fclose(f);
        return AllocFailed;
    }
    fread(hdr, sizeof(PBFS_Header), 1, f);

    PBFS_Permission_Flags perm_flags = parse_file_perms(perms);
    PBFS_Metadata_Flags meta_flags = parse_file_type(type);
    if (perm_flags == PERM_INVALID) {
        fprintf(stderr, "Invalid Permission Flags: %s\n", perms);
        fclose(f);
        free(hdr);
        return InvalidArgument;
    } else if (meta_flags == METADATA_FLAG_INVALID) {
        fprintf(stderr, "Invalid file type: %s\n", type);
        fclose(f);
        free(hdr);
        return InvalidArgument;
    }

    FILE* file_to_add = fopen(file, "rb");
    if (!f) {
        perror("Failed to open file!\n");
        fclose(f);
        free(hdr);
        return FileError;
    }

    fseek(file_to_add, 0, SEEK_END);
    size_t data_size = ftell(file_to_add);
    rewind(file_to_add);
    size_t aligned_data_size = align_up(data_size, hdr->block_size);
    uint8_t* data = (uint8_t*)calloc(aligned_data_size, 1);
    if (!data) {
        perror("Failed to Allocate memory!\n");
        fclose(file_to_add);
        fclose(f);
        free(hdr);
        return AllocFailed;
    }
    fread(data, 1, data_size, file_to_add);
    fclose(file_to_add);

    out = pbfs_add_ex(f, data, aligned_data_size, hdr, name, uid, gid, meta_flags, perm_flags);
    fclose(f);
    free(hdr);
    free(data);
    if (out != EXIT_SUCCESS) {
        fprintf(stderr, "Failed To Add File!\n");
        return out;
    }
    return EXIT_SUCCESS;
}

// Removes a file from the disk
int pbfs_remove(const char* image_path, const char* name) {
    FILE* fp = fopen(image_path, "rb+");
    if (!fp) return FileError;

    PBFS_Header hdr;
    fseek(fp, 512, SEEK_SET);
    fread(&hdr, sizeof(PBFS_Header), 1, fp);

    PBFS_Bitmap bitmap;
    fseek(fp, LBA(hdr.bitmap_lba, hdr.block_size), SEEK_SET);
    fread(&bitmap, sizeof(PBFS_Bitmap), 1, fp);

    PBFS_DMM dmm;
    uint128_t cur_dmm_lba = uint128_from_u64(hdr.dmm_root_lba);
    
    while (UINT128_GT(cur_dmm_lba, UINT128_ZERO)) {
        fseek(fp, LBA(uint128_to_u64(cur_dmm_lba), hdr.block_size), SEEK_SET);
        fread(&dmm, sizeof(PBFS_DMM), 1, fp);

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (strcmp(dmm.entries[i].name, name) == 0) {               
                // Check for protection flag
                if (dmm.entries[i].perms & PERM_LOCKED) {
                    fprintf(stderr, "Error: File is locked and cannot be deleted.\n");
                    fclose(fp);
                    return PermissionDenied;
                }

                // Execute Deletion
                if (dmm.entries[i].type & METADATA_FLAG_DIR) {
                    pbfs_delete_recursive(fp, &hdr, &bitmap, dmm.entries[i].lba);
                } else {
                    pbfs_free_file_blocks(fp, &hdr, &bitmap, dmm.entries[i].lba);
                }

                // Remove entry from current DMM block by shifting
                for (uint32_t j = i; j < dmm.entry_count - 1; j++) {
                    dmm.entries[j] = dmm.entries[j+1];
                }
                dmm.entry_count--;

                // Update the DMM block on disk
                fseek(fp, LBA(uint128_to_u64(cur_dmm_lba), hdr.block_size), SEEK_SET);
                fwrite(&dmm, sizeof(PBFS_DMM), 1, fp);
                
                fclose(fp);
                return EXIT_SUCCESS;
            }
        }
        cur_dmm_lba = dmm.extender_lba;
    }

    fclose(fp);
    return FileNotFound;
}

// Lists a dir from the disk
int pbfs_list_dir(const char* image_path, const char* dir_path) {
    FILE* fp = fopen(image_path, "rb");
    if (!fp) {
        perror("List failed: Could not open image");
        return FileError;
    }

    PBFS_Header hdr;
    fseek(fp, 512, SEEK_SET);
    if (fread(&hdr, sizeof(PBFS_Header), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp); return InvalidHeader;
    }

    PBFS_DMM dmm;
    uint128_t cur_dmm_lba = uint128_from_u64(hdr.dmm_root_lba);
    int found = 0;

    // Search the entire DMM chain for the path name
    while (UINT128_GT(cur_dmm_lba, UINT128_ZERO)) {
        fseek(fp, LBA(uint128_to_u64(cur_dmm_lba), hdr.block_size), SEEK_SET);
        if (fread(&dmm, sizeof(PBFS_DMM), 1, fp) != 1) break;

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (strcmp(dmm.entries[i].name, dir_path) == 0) {
                found = 1;
                const char* type_name = file_type_to_str(dmm.entries[i].type);

                if (dmm.entries[i].type & METADATA_FLAG_DIR) {
                    printf("Contents of Directory [%s]:\n", dir_path);
                    // TODO: Make a Directory DMM
                    PBFS_DMM sub_dmm;
                    fseek(fp, LBA(uint128_to_u64(dmm.entries[i].lba), hdr.block_size), SEEK_SET);
                    if (fread(&sub_dmm, sizeof(PBFS_DMM), 1, fp) == 1) {
                        for (uint32_t j = 0; j < sub_dmm.entry_count; j++) {
                            printf(
                                "  %-16s  %s\n", sub_dmm.entries[j].name, 
                                file_type_to_str(sub_dmm.entries[j].type)
                            );
                        }
                    }
                } else {
                    // It's a file, just show the entry info
                    printf(
                        "%-16s  %s  LBA: %llu\n", dmm.entries[i].name, type_name, 
                        (unsigned long long)uint128_to_u64(dmm.entries[i].lba)
                    );
                }
                break;
            }
        }
        if (found) break;
        cur_dmm_lba = dmm.extender_lba;
    }

    if (!found) fprintf(stderr, "Error: Path '%s' not found in DMM.\n", dir_path);
    fclose(fp);
    return found ? EXIT_SUCCESS : FileNotFound;
}

// Updates a file on the disk
int pbfs_update_file(const char* image_path, const char* name, uint8_t* new_data, size_t new_size) {
    FILE* fp = fopen(image_path, "rb+");
    if (!fp) { perror("Update failed"); return FileError; }

    PBFS_Header hdr;
    fseek(fp, 512, SEEK_SET);
    fread(&hdr, sizeof(PBFS_Header), 1, fp);

    PBFS_Bitmap bitmap;
    fseek(fp, LBA(hdr.bitmap_lba, hdr.block_size), SEEK_SET);
    fread(&bitmap, sizeof(PBFS_Bitmap), 1, fp);

    PBFS_DMM dmm;
    uint128_t cur_dmm_lba = uint128_from_u64(hdr.dmm_root_lba);
    
    while (UINT128_GT(cur_dmm_lba, UINT128_ZERO)) {
        fseek(fp, LBA(uint128_to_u64(cur_dmm_lba), hdr.block_size), SEEK_SET);
        fread(&dmm, sizeof(PBFS_DMM), 1, fp);

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (strcmp(dmm.entries[i].name, name) == 0) {
                PBFS_Metadata meta;
                uint64_t meta_lba = uint128_to_u64(dmm.entries[i].lba);
                fseek(fp, LBA(meta_lba, hdr.block_size), SEEK_SET);
                fread(&meta, sizeof(PBFS_Metadata), 1, fp);

                uint64_t old_size = uint128_to_u64(meta.data_size);
                uint64_t old_blocks = (old_size + hdr.block_size - 1) / hdr.block_size;
                uint64_t new_blocks = (new_size + hdr.block_size - 1) / hdr.block_size;

                size_t first_chunk = (new_size < old_size) ? new_size : old_size;
                fseek(fp, LBA(meta_lba, hdr.block_size) + sizeof(PBFS_Metadata), SEEK_SET);
                fwrite(new_data, 1, first_chunk, fp);

                if (new_size > old_size) {
                    uint128_t ext_lba = bitmap_find_blocks(&bitmap, uint128_from_u32(1), fp, hdr.block_size);
                    if (uint128_is_zero(&ext_lba)) {
                        fprintf(stderr, "Insufficient space for file extension!\n");
                        fclose(fp); return NoSpaceAvailable;
                    }

                    // Write overflow data to the extension block
                    fseek(fp, LBA(uint128_to_u64(ext_lba), hdr.block_size), SEEK_SET);
                    fwrite(new_data + first_chunk, 1, new_size - first_chunk, fp);
                    
                    // Update original metadata to point to extension
                    uint128_t ext = UINT128_ZERO;
                    uint128_sub(&ext, &ext_lba, &dmm.entries[i].lba);
                    meta.extender_lba = ext;
                    bitmap_set(&bitmap, ext_lba, uint128_from_u64(hdr.bitmap_lba), UINT128_ZERO, fp, hdr.block_size, 1);
                }

                meta.data_size = uint128_from_u64(new_size);
                meta.modified_timestamp = (uint64_t)time(NULL);
                fseek(fp, LBA(meta_lba, hdr.block_size), SEEK_SET);
                fwrite(&meta, sizeof(PBFS_Metadata), 1, fp);

                fclose(fp);
                return EXIT_SUCCESS;
            }
        }
        cur_dmm_lba = dmm.extender_lba;
    }
    fclose(fp);
    return FileNotFound;
}

// Adds a dir
int pbfs_add_dir(const char* image, char* name, char* perms, uint32_t gid, uint32_t uid) {
    FILE* f = fopen(image, "rb+");
    if (!f) {
        perror("Failed to open image for directory creation");
        return FileError;
    }

    PBFS_Header hdr;
    fseek(f, 512, SEEK_SET);
    if (fread(&hdr, sizeof(PBFS_Header), 1, f) != 1) {
        fprintf(stderr, "Error: Failed to read image header\n");
        fclose(f); return InvalidHeader;
    }

    PBFS_Permission_Flags perm_flags = parse_file_perms(perms);
    if (perm_flags == PERM_INVALID) {
        fprintf(stderr, "Error: Invalid permission flags '%s'\n", perms);
        fclose(f); return InvalidArgument;
    }

    PBFS_Bitmap bitmap;
    fseek(f, LBA(hdr.bitmap_lba, hdr.block_size), SEEK_SET);
    fread(&bitmap, sizeof(PBFS_Bitmap), 1, f);

    // Find a block for the new directory's Metadata
    uint128_t dir_lba = bitmap_find_blocks(&bitmap, uint128_from_u32(1), f, hdr.block_size);
    if (uint128_is_zero(&dir_lba)) {
        fprintf(stderr, "Error: No space available for new directory Metadata\n");
        fclose(f); return NoSpaceAvailable;
    }

    // Initialize the new empty Metadata block
    PBFS_Metadata new_md = {0};
    new_md.data_offset = 0;
    new_md.data_size = UINT128_ZERO;
    new_md.extender_lba = UINT128_ZERO;
    new_md.ex_flags = perm_flags;
    new_md.flags = METADATA_FLAG_DIR;
    new_md.gid = gid;
    new_md.uid = uid;
    new_md.permission_offset = 0;
    time_t now = {0};
    time(&now);
    new_md.created_timestamp = (uint64_t)now;
    new_md.modified_timestamp = (uint64_t)now;
    fseek(f, LBA(uint128_to_u64(dir_lba), hdr.block_size), SEEK_SET);
    fwrite(&new_md, sizeof(PBFS_Metadata), 1, f);

    // Mark the block as used
    bitmap_set(&bitmap, dir_lba, uint128_from_u64(hdr.bitmap_lba), UINT128_ZERO, f, hdr.block_size, 1);

    // Add entry to the root/parent DMM
    int out = pbfs_add_dmm_entry(f, &hdr, &bitmap, dir_lba, name, METADATA_FLAG_DIR, perm_flags, (uint64_t)now, (uint64_t)now);
    
    fclose(f);
    if (out != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Failed to add directory entry to DMM\n");
    }
    return out;
}

// Reads a file
int pbfs_read_file(const char* image_path, const char* name, uint8_t** out_buffer, size_t* out_size) {
    FILE* fp = fopen(image_path, "rb");
    if (!fp) {
    perror("Read failed: Could not open image");
    return FileError;
    }

    PBFS_Header hdr;
    fseek(fp, 512, SEEK_SET);
    if (fread(&hdr, sizeof(PBFS_Header), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp); return InvalidHeader;
    }

    // Locate the file
    PBFS_DMM dmm;
    uint128_t cur_dmm_lba = uint128_from_u64(hdr.dmm_root_lba);
    uint128_t file_lba = UINT128_ZERO;

    while (UINT128_GT(cur_dmm_lba, UINT128_ZERO)) {
        fseek(fp, LBA(uint128_to_u64(cur_dmm_lba), hdr.block_size), SEEK_SET);
        if (fread(&dmm, sizeof(PBFS_DMM), 1, fp) != 1) break;

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            if (strcmp(dmm.entries[i].name, name) == 0) {
                if (dmm.entries[i].type & METADATA_FLAG_DIR) {
                    fprintf(stderr, "Error: '%s' is a directory.\n", name);
                    fclose(fp); return InvalidArgument;
                }
                file_lba = dmm.entries[i].lba;
                break;
            }
        }
        if (UINT128_GT(file_lba, UINT128_ZERO)) break;
        cur_dmm_lba = dmm.extender_lba;
    }

    if (uint128_is_zero(&file_lba)) {
        fprintf(stderr, "Error: File '%s' not found.\n", name);
        fclose(fp); return FileNotFound;
    }

    PBFS_Metadata meta;
    fseek(fp, LBA(uint128_to_u64(file_lba), hdr.block_size), SEEK_SET);
    fread(&meta, sizeof(PBFS_Metadata), 1, fp);

    size_t total_size = (size_t)uint128_to_u64(meta.data_size);
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) {
        perror("Failed to allocate read buffer");
        fclose(fp); return AllocFailed;
    }

    // Chain Reading
    size_t bytes_read = 0;
    uint128_t next_lba = file_lba;

    while (bytes_read < total_size && UINT128_GT(next_lba, UINT128_ZERO)) {
        uint64_t current_raw_lba = uint128_to_u64(next_lba);
        
        fseek(fp, LBA(current_raw_lba, hdr.block_size), SEEK_SET);
        if (fread(&meta, sizeof(PBFS_Metadata), 1, fp) != 1) break;

        size_t remaining = total_size - bytes_read;
        size_t base_capacity = hdr.block_size - sizeof(PBFS_Metadata);
        size_t chunk_size = (remaining < base_capacity) ? remaining : base_capacity;
        
        fseek(fp, LBA(current_raw_lba, hdr.block_size) + sizeof(PBFS_Metadata), SEEK_SET);
        fread(buffer + bytes_read, 1, chunk_size, fp);
        
        bytes_read += chunk_size;
        next_lba = meta.extender_lba; 
    }

    *out_buffer = buffer;
    *out_size = total_size;

    fclose(fp);
    return EXIT_SUCCESS;
}

// Main function
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pbfs-cli <image> <[commands]>\nCommands:\n\t-bs/--block_size: Define the Block Size\n\t-tb/--total_blocks: Define Total number of blocks\n\t-dn/--disk_name: Define the disk name\n\t... USE -h/--help for more info.\n");
        return InvalidUsage;
    }

    uint8_t create_image = 0;
    uint8_t format_image = 0;

    uint8_t add_file = 0;
    uint8_t add_dir = 0;
    uint8_t add_bootloader = 0;
    uint8_t remove_file = 0;
    uint8_t update_file = 0;

    uint8_t list_file = 0;
    uint8_t read_file = 0;
    uint8_t read_file_binary = 0;

    uint8_t bootpart = 0;
    int bootpartsize = 0;

    uint8_t kerneltable = 0;
    uint8_t add_kernel = 0;

    uint8_t test_image = 0;

    uint32_t block_size = 512;
    uint128_t total_blocks = uint128_from_u32(2048);
    uint128_t disk_size = uint128_from_u32(1048576);
    uint128_t volume_id = UINT128_ZERO;
    char disk_name[32];

    char name[64];
    uint32_t gid = 0;
    uint32_t uid = 0;
    char perms[6];
    char type[6];

    char* filepath = "";

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      printf(PBFS_CLI_HELP);
      return EXIT_SUCCESS;
    }

    char* image = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-bs") == 0 || strcmp(argv[i], "--block_size") == 0) {
            block_size = (uint32_t)atoi(argv[i + 1]);
            if (block_size < 512) {
                fprintf(stderr, "Block Size cannot be lower than 512!\n");
                return InvalidUsage;
            }
            // Update the disk size
            uint128_mul_u32(&disk_size, &total_blocks, block_size);
            i++;
        } else if (strcmp(argv[i], "-tb") == 0 || strcmp(argv[i], "--total_blocks") == 0) {
            total_blocks = uint128_from_u64((uint64_t)atoll(argv[i + 1]));
            // Update the disk size
            uint128_mul_u32(&disk_size, &total_blocks, block_size);
            i++;
        } else if (strcmp(argv[i], "-dn") == 0 || strcmp(argv[i], "--disk_name") == 0) {
            strncpy(disk_name, argv[i + 1], 31);
            disk_name[31] = '\0';
            i++;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--volume_id") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <volume_id>\n", argv[i]);
                return InvalidUsage;
            }
            volume_id = uint128_from_u64((uint64_t)atoll(argv[i + 1]));
            i++;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            format_image = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(PBFS_CLI_HELP);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--create") == 0) {
            // Create the image
            create_image = 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--add") == 0) {
            // Add a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            add_file = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--remove") == 0) {
            // Removes a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            remove_file = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--update") == 0) {
            // Add a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            update_file = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-ad") == 0 || strcmp(argv[i], "--add_dir") == 0) {
            // Add a dir
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                return InvalidUsage;
            }
            add_dir = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            // List a path
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                return InvalidUsage;
            }
            list_file = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-rf") == 0 || strcmp(argv[i], "--read_file") == 0) {
            // Read a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                return InvalidUsage;
            }
            read_file = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-rfb") == 0 || strcmp(argv[i], "--read_file_binary") == 0) {
            // Read a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                return InvalidUsage;
            }
            read_file_binary = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--gid") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <gid>\n", argv[i]);
                return InvalidUsage;
            }
            gid = (uint32_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--uid") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <uid>\n", argv[i]);
                return InvalidUsage;
            }
            uid = (uint32_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--name") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <name>\n", argv[i]);
                return InvalidUsage;
            }
            strncpy(name, argv[i + 1], sizeof(name));
            i++;
        } else if (strcmp(argv[i], "--permissions") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <permissions>\n", argv[i]);
                return InvalidUsage;
            }
            strncpy(perms, argv[i + 1], sizeof(perms));
            i++;
        } else if (strcmp(argv[i], "--type") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <type>\n", argv[i]);
                return InvalidUsage;
            }
            strncpy(type, argv[i + 1], sizeof(type));
            i++;
        } else if (strcmp(argv[i], "-btl") == 0 || strcmp(argv[i], "--bootloader") == 0) {
            // Add a bootloader
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            add_bootloader = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-btp") == 0 || strcmp(argv[i], "--boot_partition") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <blocks>\n", argv[i]);
                return InvalidUsage;
            }
            bootpartsize = (int)atoi(argv[i + 1]);
            bootpart = 1;
            i++;
        } else if (strcmp(argv[i], "-rkt") == 0 || strcmp(argv[i], "--reserve_kernel_table") == 0) {
            kerneltable = 1;
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kernel") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            add_kernel = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            test_image = 1;
        } else {
            printf("Unknown command: %s\n\tTry using -h/--help for more info!\n", argv[i]);
            return InvalidArgument;
        }
    }

    if (create_image) {
        printf("Creating Image...\n");
        int out = pbfs_create(image, block_size, total_blocks);

        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (format_image) {
        printf("Formating Image...\n");
        int out = pbfs_format(image, kerneltable, bootpart, bootpartsize, block_size, total_blocks, disk_name, volume_id);

        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (add_file) {
        if (strlen(filepath) < 1 || strlen(name) < 1) {
            strcpy(name, filepath);
        }
        printf("Adding File [%s] to Image...\n", name);
        int out = pbfs_add(image, filepath, name, perms, type, gid, uid);
        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (add_dir) {
        printf("Adding Dir [%s] to Image...\n", filepath);
        int out = pbfs_add_dir(image, filepath, perms, gid, uid);
        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (remove_file) {
        if (strlen(filepath) < 1 || strlen(name) < 1) {
            strcpy(name, filepath);
        }
        printf("Removing File [%s] from Image...\n", filepath);
        int out = pbfs_remove(image, filepath);
        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (update_file) {
        if (strlen(filepath) < 1 || strlen(name) < 1) {
            strcpy(name, filepath);
        }
        printf("Updating File [%s] from Image...\n", name);
        FILE* f = fopen(filepath, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open file %s. ", filepath);
            perror("Failed to open file!\n");
            return FileError;
        }
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        rewind(f);
        uint8_t* data = (uint8_t*)malloc(size);
        if (!data) {
            perror("Failed to allocate memory\n");
            fclose(f);
            return AllocFailed;
        }
        fread(data, size, 1, f);
        fclose(f);
        int out = pbfs_update_file(image, name, data, size);
        if (out != EXIT_SUCCESS) {
            free(data);
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        free(data);
        printf("Done!\n");
    }
    else if (add_bootloader) {
        printf("Adding bootloader [%s] to Image...\n", filepath);
        printf("Done!\n");
    }
    else if (add_kernel) {
        printf("Adding Kernel [%s]...\n", filepath); 
        printf("Done!\n");
    }
    else if (test_image) {
        int out = pbfs_test(image, 1);
        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
    } 
    else if (list_file) {
        printf("Listing [%s] from Image...\n", filepath);
        int out = pbfs_list_dir(image, filepath);
        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    else if (read_file_binary) {
        printf("Reading [%s] from Image...\n", filepath);
        uint8_t* data = NULL;
        size_t data_size = 0;
        int out = pbfs_read_file(image, filepath, &data, &data_size);
        if (out != EXIT_SUCCESS) {
            if (data) free(data);
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        for (size_t i = 0; i < data_size; i++) {
            printf("%02X", data[i]);
            if (i < data_size - 1) {
                printf(" ");
            }
        }
        free(data);
        printf("\nDone!\n");
    }
    else if (read_file) {
        printf("Reading [%s] from Image...\n", filepath);
        uint8_t* data = NULL;
        size_t data_size = 0;
        int out = pbfs_read_file(image, filepath, &data, &data_size);
        if (out != EXIT_SUCCESS) {
            if (data) free(data);
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        for (size_t i = 0; i < data_size; i++) {
            if (data[i] == '\0') break;
            printf("%c", (char)data[i]);
        }
        free(data);
        printf("\nDone!\n");
    }

    return EXIT_SUCCESS;
}
