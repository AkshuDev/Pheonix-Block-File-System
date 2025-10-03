#define CLI

#include "pbfs.h"
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
int pbfs_format(const char* image) {
    FILE* fp = fopen(image, "wb");
    if (!fp) return FileError;

    // Allocate Blocks
    uint64_t* disk = calloc(1, block_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }

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

    header->FileTableOffset = (block_size * 2) + bitmap_size;
    header->Entries = 0;

    // Write header
    fwrite(disk, 1, block_size, fp);

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

    int create_image = 0;
    int format_image = 0;

    int add_file = 0;
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
        int out = pbfs_format(image);

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
            return -1;
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
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

    return EXIT_SUCCESS;
}
