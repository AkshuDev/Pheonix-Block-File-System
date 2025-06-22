#include "pbfs-cli.h"
#include "pbfs_structs.h"

int block_size = 512; // The size of a single block in bytes
int total_blocks = 2048; // Total number of blocks
int disk_size = 512 * 2048;
char disk_name[24] = "PBFS_DISK\0\0";

// Verifies the header
int pbfs_verify_header(PBFS_Header* header) {
    if (header->Magic[0] != 'P' || header->Magic[1] != 'B' || header->Magic[2] != 'F' || header->Magic[3] != 'S' || header->Magic[4] != '\0' || header->Magic[5] != '\0') return InvalidHeader;
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
    memcpy(header->Magic, "PBFS\0\0", 6);
    header->Block_Size = block_size;
    header->Total_Blocks = total_blocks;
    strncpy(header->Disk_Name, disk_name, 24);
    header->Timestamp = (uint64_t)time(NULL);
    header->First_Boot_Timestamp = (uint64_t)0;
    header->Version = 1;
    header->OS_BootMode = 0;

    header->FileTableOffset = 2 * block_size;
    header->Entries = 0;

    // Write layout info
    PBFS_Layout layout = {
        .Header_Start = 1,
        .Header_End = 1,
        .Header_BlockSpan = 1,
        .FileTable_Start = 2,
        .FileTable_BlockSpan = 1,
        .FileTable_End = 2,
        .PermissionTable_Start = 3,
        .PermissionTable_BlockSpan = 1,
        .PermissionTable_End = 3,
        .FileTree_Start = 4,
        .FileTable_BlockSpan = 4,
        .FileTree_End = 4,
        .Bitmap_Start = 5,
        .Bitmap_BlockSpan = 1,
        .Bitmap_End = 5,
        .Data_Start = 10 // Allocate 5 blocks for ease of use.
    }; // Just there for testing

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

// Adds a file to the image
int pbfs_add_file(const char* image, const char* filename, const char* datapath, PBFS_Permissions* permissions, char** permission_granted_files) {
    // Step 1. Verify header
    // Step 2. Read the file table and append the entry
    // Step 3. Read the permission table and append the entry
    // Step 4. Read the file tree and append the entry (if granted files are null that means all files are granted hence just append ~.)
    // Step 5. Read the bitmap and append the entry
    // Step 6. Read the data and append the entry

    // Open file
    FILE* fp = fopen(datapath, "rb");
    if (!fp) return FileError;

    // Verify header
    PBFS_Header header;
    fread(&header, sizeof(PBFS_Header), 1, fp);
    if (pbfs_verify_header(&header) != EXIT_SUCCESS) return InvalidHeader;

    // Read file table
    PBFS_FileTableEntry** file_table = calloc(header.Entries, sizeof(PBFS_FileTableEntry*));
    if (!file_table) return AllocFailed;

    for (int i = 0; i < header.Entries; i++) {
        fread(file_table[i], sizeof(PBFS_FileTableEntry), 1, fp);
    }

    // Read permission table
    PBFS_PermissionTableEntry** permission_table = calloc(header.Entries, sizeof(PBFS_PermissionTableEntry*));
    if (!permission_table) return AllocFailed;

    for (int i = 0; i < header.Entries; i++) {
        fread(permission_table[i], sizeof(PBFS_PermissionTableEntry), 1, fp);
    }

    // Read file tree
    PBFS_FileTreeEntry** file_tree = calloc(header.Entries, sizeof(PBFS_FileTreeEntry*));
    if (!file_tree) return AllocFailed;

    for (int i = 0; i < header.Entries; i++) {
        fread(file_tree[i], sizeof(PBFS_FileTreeEntry), 1, fp);
    }
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
        } else if (strncmp(argv[i], "-tb", 3) == 0) {
            total_blocks = atoi(argv[i + 1]);
            // Update the disk size
            disk_size = block_size * total_blocks;
        } else if (strncmp(argv[i], "-dn", 3) == 0) {
            strncpy(disk_name, argv[i + 1], 24);
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
        int out = pbfs_add_file(image, filename, filepath, permissions, permission_granted_files);

        if (out != EXIT_SUCCESS) {
            perror("An Error Occurred!\n");
            return out;
        }
    }

    return EXIT_SUCCESS;
}