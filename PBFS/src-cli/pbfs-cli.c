#include "pbfs.h"
#include "pbfs_structs.h"
#include "disk_utils.h"

#include <time.h>

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

    // Allocate full disk
    uint64_t* disk = calloc(1, disk_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }

    // Fill the header
    PBFS_Header* header = (PBFS_Header*)disk;
    memcpy(header->Magic, PBFS_MAGIC, 6);
    header->Block_Size = block_size;
    header->Total_Blocks = total_blocks;
    strncpy(header->Disk_Name, disk_name, 24);
    header->Timestamp = (uint64_t)time(NULL);
    header->First_Boot_Timestamp = (uint64_t)0;
    header->Version = 1;
    header->OS_BootMode = 0;

    header->FileTableOffset = 2 * block_size;
    header->Entries = 0;

    memcpy(disk + (block_size * 1), &header, sizeof(PBFS_Header));
    // Write File Table now
    // Nothing for now
    fwrite(disk, 1, disk_size, fp);

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
    uint64_t* disk = calloc(1, disk_size);
    if (!disk) {
        fclose(fp);
        return AllocFailed;
    }

    // Now write zeroes
    memset(disk, 0, disk_size);
    fwrite(disk, 1, disk_size, fp);

    free(disk);
    fclose(fp);
    return EXIT_SUCCESS;
}


// Main function
int main(int argc, char** argv) {
    if (argc < 2) {
        perror("Usage: pbfs-cli <image> <commands>\nCommands:\n\t-bs: Define the Block Size\n\t-tb: Define Total number of blocks\n\t-dn: Define the disk name\n\t-f: Format the disk\n\t... USE -help for more info.\n");
        return InvalidUsage;
    }

    int create_image = 0;
    int format_image = 0;

    int add_file = 0;
    char* filename = "";
    char* filepath = "";
    PBFS_Permissions* permissions = NULL;
    char** permission_granted_files = NULL;

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
            if (argv[i + 1] == NULL || argv[i + 2] == NULL) {
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
        int out = pbfs_create(image);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
    }
    if (format_image) {
        int out = pbfs_format(image);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
    }
    if (add_file) {
        int out = PBFS_WRITE(filename, filepath, permissions, permission_granted_files, 1, 0x80);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
    }

    return EXIT_SUCCESS;
}