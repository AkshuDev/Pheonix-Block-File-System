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

// Verifies the header
int pbfs_verify_header(PBFS_Header* header) {
    if (!memcmp(&header->magic, PBFS_MAGIC, PBFS_MAGIC_LEN) != 0) return HeaderVerificationFailed;
    return EXIT_SUCCESS;
}

// Creates and Formats the disk
int pbfs_format(const char* image, uint8_t kernel_table, uint8_t bootpartition, uint8_t bootpart_size, uint32_t block_size, uint128_t total_blocks) {
    if (bootpartition <= 1) {
        fprintf(stderr, "Error: Sorry Boot partition cannot be lower or equal to LBA 1 [PBFS Header + MBR/GPT]\n");
        return InvalidArgument;
    }

    FILE* fp = fopen(image, "wb");
    if (!fp) return FileError;

    // Allocate Blocks
    uint8_t* disk = calloc(1, block_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }
    
    // Write MBR/GPT
    fseek(fp, 0, SEEK_SET);
    fwrite(disk, block_size, 1, fp);
    // Set up Header
    PBFS_Header* hdr = (PBFS_Header*)disk;
    memset(hdr, 0, sizeof(PBFS_Header));

    memcpy(&hdr->magic, PBFS_MAGIC, PBFS_MAGIC_LEN);
    hdr->block_size = block_size;
    hdr->total_blocks = total_blocks;
    
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
    }
    if (bootpartition == 1) {
        boot_part_lba = data_lba;
        data_lba += bootpart_size;
    }

    hdr->bitmap_lba = bitmap_lba;
    hdr->kernel_table_lba = kernel_table_lba;
    hdr->dmm_root_lba = dmm_lba;
    hdr->sysinfo_lba = sysinfo_lba;
    hdr->boot_partition_lba = boot_part_lba;
    hdr->data_start_lba = data_lba;

    fseek(fp, LBA(1, block_size), SEEK_SET);
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
    for (uint128_t i = UINT128_ZERO; uint128_cmp(&i, &total_blocks_idx) == 0; uint128_inc(&i)) {
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
    for (uint128_t i = UINT128_ZERO; uint128_cmp(&i, &total_blocks_idx) == 0; uint128_inc(&i)) {
        fwrite(disk, block_size, 1, fp);
    }

    free(disk);
    fclose(fp);
    return EXIT_SUCCESS;
}


// Main function
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pbfs-cli <image> <commands>\nCommands:\n\t-bs: Define the Block Size\n\t-tb: Define Total number of blocks\n\t-dn: Define the disk name\n\t-f: Format the disk\n\t... USE -help for more info.\n");
        return InvalidUsage;
    }

    uint8_t create_image = 0;
    uint8_t format_image = 0;
    uint8_t add_file = 0;
    uint8_t add_bootloader = 0;
    uint8_t bootpart = 0;
    uint8_t kerneltable = 0;
    uint8_t add_kernel = 0;
    uint32_t block_size = 0;
    uint128_t total_blocks = UINT128_ZERO;
    uint128_t disk_size = UINT128_ZERO;

    char* disk_name = NULL;

    int bootpartsize = 0;

    char* filename = "";
    char* filepath = "";

    if (strncmp(argv[1], "-help", 5) == 0) {
      printf(PBFS_CLI_HELP);
      return EXIT_SUCCESS;
    }

    char* image = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-bs", 3) == 0) {
            block_size = (uint32_t)atoi(argv[i + 1]);
            // Update the disk size
            uint128_mul_u32(&disk_size, &total_blocks, block_size);
            i++;
        } else if (strncmp(argv[i], "-tb", 3) == 0) {
            total_blocks = uint128_from_u64((uint64_t)atoll(argv[i + 1]));
            // Update the disk size
            uint128_mul_u32(&disk_size, &total_blocks, block_size);
            i++;
        } else if (strncmp(argv[i], "-dn", 3) == 0) {
            strncpy(disk_name, argv[i + 1], 32);
            i++;
        } else if (strncmp(argv[i], "-f", 2) == 0) {
            format_image = 1;
        } else if (strncmp(argv[i], "-help", 5) == 0) {
            printf(PBFS_CLI_HELP);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[i], "-c", 2) == 0) {
            // Create the image
            create_image = 1;
        } else if (strncmp(argv[i], "-add", 4) == 0) {
            // Add a file
            // Check if the filename and filepath are also provided
            if (i + 3 > argc) {
                printf("Usage: pbfs-cli <image> -add <filename> <filepath>\n");
                return InvalidUsage;
            }
            add_file = 1;
            filename = argv[i + 1];
            filepath = argv[i + 2];
            i++;
            i++;
        } else if (strncmp(argv[i], "-bootloader", 11) == 0) {
            // Add a bootloader
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> -bootloader <filepath>\n");
                return InvalidUsage;
            }
            add_bootloader = 1;
            filepath = argv[i + 1];
            i++;
        } else if (strncmp(argv[i], "-partboot", 9) == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> -partboot <blocks>\n");
                return InvalidUsage;
            }
            bootpartsize = (int)argv[i + 1];
            bootpart = 1;
            i++;
        } else if (strcmp(argv[i], "-partkernel") == 0) {
            kerneltable = 1;
        } else if (strncmp(argv[i], "-kernel", 7) == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> -kernel <filepath>\n");
                return InvalidUsage;
            }
            add_kernel = 1;
            filepath = argv[i + 1];
            i++;
        } else {
            printf("Unknown command: %s\n", argv[i]);
            return InvalidArgument;
        }
    }

    printf("Block Size: %d\n", block_size);
    printf("Total Blocks: %d\n", total_blocks);
    printf("Disk Name: %s\n", disk_name);

    if (create_image) {
        printf("Creating Image...\n");
        int out = pbfs_create(image, block_size, total_blocks);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
    }
    if (format_image) {
        printf("Formating Image...\n");
        int out = pbfs_format(image, kerneltable, bootpart, bootpartsize, block_size, total_blocks);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
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

    return EXIT_SUCCESS;
}
