#define CLI
#include <pbfs.h>
#include <pbfs-cli.h>
#include <pbfs_structs.h>
#undef CLI

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

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

    fseek(fp, LBA(ext_lba, block_size), SEEK_SET);
    if (fread(current, sizeof(PBFS_Bitmap), 1, fp) == 0) return -1;
    uint128_t bitmap_idx_inced = bitmap_idx;
    uint128_inc(&bitmap_idx_inced);
    return bitmap_test_recursive(current, lba, bitmap_idx_inced, fp, block_size);
}
uint128_t bitmap_set(
    PBFS_Bitmap* current, 
    uint128_t lba, 
    uint128_t current_lba, 
    uint128_t bitmap_idx, 
    FILE* fp,
    int block_size,
    uint8_t value // 0/1 (default: 0)
) {
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

    if (next_ext_lba <= 1) {
        // Find a free block in CURRENT bitmap to use as the NEW extension block
        // use the first available bit in the current data range
        uint128_t new_block = UINT128_ZERO;
        for (uint64_t i = 0; i < (PBFS_BITMAP_LIMIT * 8); i++) {
            if (!bitmap_bit_test(current->bytes, i)) {
                uint128_mul_u32(&new_block, &bitmap_idx, PBFS_BITMAP_LIMIT * 8);
                uint128_t i_128 = uint128_from_u64(i);
                uint128_add(&new_block, &new_block, &i_128);
                break;
            }
        }

        if (uint128_is_zero(&new_block)) return UINT128_ZERO; // Truly out of space

        // Link current to new
        current->extender_lba = new_block;
        fseek(fp, LBA(uint128_to_u64(current_lba), block_size), SEEK_SET);
        fwrite(current, sizeof(PBFS_Bitmap), 1, fp);

        // Initialize new bitmap block
        PBFS_Bitmap new_bitmap;
        memset(&new_bitmap, 0, sizeof(PBFS_Bitmap));
        fseek(fp, LBA(uint128_to_u64(new_block), block_size), SEEK_SET);
        fwrite(&new_bitmap, sizeof(PBFS_Bitmap), 1, fp);

        // Continue recursion with the newly created block
        memcpy(current, &new_bitmap, sizeof(PBFS_Bitmap));
        next_ext = new_block;
    } else {
        fseek(fp, LBA(next_ext_lba, block_size), SEEK_SET);
        if (fread(current, sizeof(PBFS_Bitmap), 1, fp) == 0) return UINT128_ZERO;
    }

    uint128_t bitmap_idx_inced = bitmap_idx;
    uint128_inc(&bitmap_idx_inced);
    return bitmap_set(current, lba, next_ext, bitmap_idx_inced, fp, block_size, value);
}

// Verifies the header
int pbfs_verify_header(PBFS_Header* header) {
    if (memcmp(&header->magic, PBFS_MAGIC, PBFS_MAGIC_LEN) != 0) return HeaderVerificationFailed;
    return EXIT_SUCCESS;
}

// Creates and Formats the disk
int pbfs_format(const char* image, uint8_t kernel_table, uint8_t bootpartition, uint8_t bootpart_size, uint32_t block_size, uint128_t total_blocks, char* disk_name, uint128_t volume_id) {
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

// Test and print Disk Info
int pbfs_test(const char* image) {
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

    printf(
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

    if (hdr->boot_partition_lba <= 1)
        printf("Info: No Boot Partition Found\n");
    else
        printf("\n=== Boot Partiton ===\nSize (In Blocks): %lld\n", (unsigned long long int)(hdr->data_start_lba - hdr->boot_partition_lba));
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

    printf("Success!\n");

    free(hdr);
    free(disk);
    fclose(f);
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
    uint8_t add_bootloader = 0;

    uint8_t bootpart = 0;
    int bootpartsize = 0;

    uint8_t kerneltable = 0;
    uint8_t add_kernel = 0;

    uint8_t test_image = 0;

    uint32_t block_size = 0;
    uint128_t total_blocks = UINT128_ZERO;
    uint128_t disk_size = UINT128_ZERO;
    uint128_t volume_id = UINT128_ZERO;
    char disk_name[32];

    char* filename = "";
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
            // Check if the filename and filepath are also provided
            if (i + 3 > argc) {
                printf("Usage: pbfs-cli <image> %s <filename> <filepath>\n", argv[i]);
                return InvalidUsage;
            }
            add_file = 1;
            filename = argv[i + 1];
            filepath = argv[i + 2];
            i++;
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
        } else if (strcmp(argv[i], "-akt") == 0 || strcmp(argv[i], "--add_kernel_table") == 0) {
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
    if (format_image) {
        printf("Formating Image...\n");
        int out = pbfs_format(image, kerneltable, bootpart, bootpartsize, block_size, total_blocks, disk_name, volume_id);

        if (out != EXIT_SUCCESS) {
            fprintf(stderr, "An Error Occurred!\n");
            return out;
        }
        printf("Done!\n");
    }
    if (add_file) {
        printf("Adding File [%s] to Image...\n", filename);
        printf("Done!\n");
    }
    if (add_bootloader) {
        printf("Adding bootloader [%s] to Image...\n", filepath);
        printf("Done!\n");
    }
    if (add_kernel) {
        printf("Adding Kernel [%s]...\n", filepath); 
        printf("Done!\n");
    }
    if (test_image) {
        pbfs_test(image);
    }

    return EXIT_SUCCESS;
}
