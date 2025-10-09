#define CLI

#include "pbfs-cli.h"
#include "pbfs_structs.h"
#include "disk_utils.h"

#include <time.h>

static size_t align_up(size_t value, size_t alignment) {
    if (alignment == 0) return value;
    size_t remainder = value % alignment;
    return remainder == 0 ? value : value + alignment - remainder;
}

// Verifies the memory header/end
int pbfs_verify_mem_header(char* header, int mode){
    // Mode = 0 -> check mem start header
    // Mode = 1 -> check mem end header
    if (mode != 0 && mode != 1) return HeaderVerificationFailed;

    if (mode == 0) {
        return (strncmp(header, MEM_MAGIC, MEM_MAGIC_LEN) == 0) ? EXIT_SUCCESS : HeaderVerificationFailed;
    }

    return (strncmp(header, MEM_END_MAGIC, MEM_END_MAGIC_LEN) == 0) ? EXIT_SUCCESS : HeaderVerificationFailed;
}

// Verifies the header
int pbfs_verify_header(PBFS_Header* header) {
    if (header->Magic[0] != 'P' || header->Magic[1] != 'B' || header->Magic[2] != 'F' || header->Magic[3] != 'S' || header->Magic[4] != '\0' || header->Magic[5] != '\0') return HeaderVerificationFailed;
    return EXIT_SUCCESS;
}

// Creates and Formats the disk
int pbfs_format(const char* image, uint8_t kernel_table, uint8_t bootpartition, uint8_t bootpart_size, int block_size, int total_blocks) {
    if (bootpartition == 1 && bootpart_size > 0xFFFFFF) { // if it is larger than 3 bytes, error
        printf("Boot Partition size is too large! Invalid, Please use a size around 200-500\n");
    }

    FILE* fp = fopen(image, "wb");
    if (!fp) return FileError;

    // Allocate Blocks
    uint64_t* disk = calloc(1, block_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }
    uint32_t file_data_offset;

    // Write Reserved block
    fwrite(disk, 1, block_size, fp);

    // Fill the header
    PBFS_Header* header = (PBFS_Header*)disk;
    memset(header, 0, block_size);
    memcpy(header->Magic, PBFS_MAGIC, 6);
    header->Block_Size = block_size;
    header->Total_Blocks = total_blocks;
    strncpy(header->Disk_Name, disk_name, 24);
    header->Timestamp = (uint64_t)time(NULL);
    header->First_Boot_Timestamp = (uint64_t)0;
    header->Version = 1;
    header->OS_BootMode = 0;

    // Bitmap stuff
    uint64_t bitmap_size = (uint64_t)align_up(total_blocks, block_size);
    uint64_t bitmap_blocks = (bitmap_size + 511) / 512;  // Round up to blocks
    file_data_offset = (block_size * 2) + bitmap_size;

    if (kernel_table == 1) {
        file_data_offset += block_size * 1;
    }

    header->FileTableOffset = file_data_offset;
    header->Entries = 0;

    // Write header
    fwrite(disk, 1, block_size, fp);

    // Write Kernel Entry
    if (kernel_table == 1) {
        memset(disk, 0, block_size);
        disk[0] = KERNEL_SIGN1;
        disk[1] = KERNEL_SIGN2;
    }

    // Write Bitmap
    memset(disk, 0, block_size);
    disk[0] = 0x07;
    fwrite(disk, 1, block_size, fp);
    disk[0] = 0x0;

    uint64_t remaining_blocks = total_blocks - (2 + bitmap_blocks);
    for (uint64_t i = 0; i < remaining_blocks; i++) {
      fwrite(disk, 1, block_size, fp);
    }

    free(disk);
    fclose(fp);
    return EXIT_SUCCESS;
}

// Creates the disk only
int pbfs_create(const char* image) {
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
    memset(disk, 0, block_size);
    for (uint64_t i = 0; i < total_blocks; i++) {
        fwrite(disk, 1, block_size, fp);
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
    int bootpartsize = 0;

    char* filename = "";
    char* filepath = "";
    PBFS_Permissions permissions = {
        .Read = 1,
        .Write = 1,
    };
    char* permission_granted_files = NULL;

    if (strncmp(argv[1], "-help", 5) == 0) {
      printf(PBFS_CLI_HELP);
      return EXIT_SUCCESS;
    }

    char* image = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-bs", 3) == 0) {
            block_size = atoi(argv[i + 1]);
            // Update the disk size
            disk_size = block_size * total_blocks;
            i++;
        } else if (strncmp(argv[i], "-tb", 3) == 0) {
            total_blocks = atoi(argv[i + 1]);
            // Update the disk size
            disk_size = block_size * total_blocks;
            i++;
        } else if (strncmp(argv[i], "-dn", 3) == 0) {
            strncpy(disk_name, argv[i + 1], 24);
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
        int out = pbfs_create(image);

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
        PBFS_SET_IMAGE_PATH(image);
        printf("Adding File [%s] to Image...\n", filename);
        FILE* fp = fopen(filepath, "rb");
        if (!fp){
            perror("Failed to open file!\n");
            return FileError;
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        if (size < 1) {
            fprintf(stderr, "File has no size!\n");
            return FileError;
        }
        rewind(fp);
        char* data = malloc(size);
        fread(data, size, 1, fp);
        fclose(fp);

        int out = PBFS_WRITE(filename, data, &permissions, &permission_granted_files, 0, 0x80);

        if (out != EXIT_SUCCESS) {
            free(data);
            perror("An Error Occurred!\n");
            return out;
        }
        free(data);
    }
    if (add_bootloader) {
        printf("Adding bootloader [%s] to Image...\n", filepath);
        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            perror("Failed to open file!\n");
            return FileError;
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        printf("Size of bootloader: %zu\n", size);
        if (size != block_size) {
            fprintf(stderr, "File doesn't match block size!\n");
            return FileError;
        }
        uint8_t* data = (uint8_t*)calloc(1, block_size);
        size_t read_bytes = fread(data, sizeof(uint8_t), size, fp); // fread might fail on binary data so we don't check for read_bytes being 0
        fclose(fp);

        printf("Read %zu bytes!\n");
        printf("First 20 bytes of file -\n");
        for (int i = 0; i < 20; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");

        FILE* disk = fopen(image, "r+b");
        if (!disk) {
            perror("Failed to open image!\n");
            return FileError;
        }

        fseek(disk, 0, SEEK_SET);
        fwrite(data, sizeof(uint8_t), block_size, disk);
        fflush(disk);
        fclose(disk);

        free(data);
    }
    if (add_kernel) {
        printf("Adding Kernel [%s]...\n", filepath);
        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            perror("Failed to open file!\n");
            return FileError;
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        printf("Size of kernel: %zu\n", size);
        uint8_t* data = (uint8_t*)calloc(1, size);
        size_t read_bytes = fread(data, sizeof(uint8_t), size, fp); // fread might fail on binary data so we don't check for read_bytes being 0
        fclose(fp);

        printf("Read %zu bytes!\n");
        printf("First 20 bytes of file -\n");
        for (int i = 0; i < 20; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");

        FILE* disk = fopen(image, "r+b");
        if (!disk) {
            perror("Failed to open image!\n");
            return FileError;
        }
        uint64_t bitmap_size = (uint64_t)align_up(total_blocks, block_size); 
        uint64_t bitmap_span = bitmap_size / block_size;
        fseek(disk, bitmap_size + (block_size * 3), SEEK_SET);
        fwrite(data, sizeof(uint8_t), size, disk);
        fseek(disk, block_size*2 + 3, SEEK_SET);
        PBFS_KernelEntry entry = {
            .name = "Kernel",
            .count = (uint16_t)align_up(size, block_size),
            .lba = bitmap_span + 3
        };

        fwrite(&entry, sizeof(entry), 1, disk);
        fclose(disk);
        printf("Done!\n");
    }

    return EXIT_SUCCESS;
}
