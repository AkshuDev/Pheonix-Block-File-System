#define PBFS_CLI
#include <pbfs.h>
#include <pbfs-cli.h>
#include <pbfs_structs.h>
#undef PBFS_CLI

#define PBFS_NDRIVERS
#ifdef PBFS_WDRIVERS
    #undef PBFS_WDRIVERS
#endif
#include <pbfs-fs.h>
#undef PBFS_NDRIVERS

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

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

    for (uint64_t i = 0; i < slen; i++) {
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
    if (size < 7) return;
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
    out[off++] = '\0';
    return;
}

static int pbfs_verify_header(PBFS_Header* hdr) {
    if (memcmp(hdr->magic, PBFS_MAGIC, PBFS_MAGIC_LEN) == 0) return PBFS_RES_SUCCESS;
    return PBFS_ERR_Invalid_Header;
}

// Lists a dir from the disk
int pbfs_list_dir(struct pbfs_mount* mnt, const char* path) {
    PBFS_DMM_Entry dir_entry = {0};
    PBFS_DMM dir_dmm = {0};
    uint64_t dir_lba = 0;
    int out = pbfs_find_entry(path, &dir_entry, &dir_lba, mnt);
    if (out != -1 && out != EXIT_SUCCESS) {
        fprintf(stderr, "No such directory: %s\n", path);
        return out;
    } else if (out == -1) {
        if (mnt->header64.dmm_root_lba <= 1) {
            fprintf(stderr, "Invalid Header!\n");
            return InvalidHeader;
        }
        dir_lba = mnt->header64.dmm_root_lba;
        pbfs_read(mnt, dir_lba, sizeof(PBFS_DMM), &dir_dmm);
    } else {
        if (!(dir_entry.type & METADATA_FLAG_DIR)) {
            fprintf(stderr, "Not a directory: %s\n", path);
            return FileNotFound;
        }

        PBFS_Metadata parent_md = {0};
        pbfs_read(mnt, uint128_to_u64(dir_entry.lba), sizeof(PBFS_Metadata), &parent_md);
        if (parent_md.data_offset < 1) {
            fprintf(stderr, "Invalid directory: %s\n", path);
            return InvalidPath;
        }
        if (!(parent_md.ex_flags & PERM_READ)) {
            char parent_perms[10];
            file_perms_to_str(parent_md.ex_flags, parent_perms, sizeof(parent_perms));
            fprintf(stderr, "Permissions Denied for directory: %s [%s]\n", path, parent_perms);
            return PermissionDenied;
        }
        dir_lba = uint128_to_u64(dir_entry.lba) + parent_md.data_offset;
        pbfs_read(mnt, uint128_to_u64(dir_entry.lba), sizeof(PBFS_DMM), &dir_dmm);
    }

    uint64_t dmm_lba = dir_lba;
    printf("Listing: %s\n", path);
    while (dmm_lba > 0) {
        PBFS_DMM dmm;
        pbfs_read(mnt, dmm_lba, sizeof(PBFS_DMM), &dmm);

        for (uint32_t i = 0; i < dmm.entry_count; i++) {
            char perms[10];
            file_perms_to_str(dmm.entries[i].perms, perms, sizeof(perms));
            printf("\t%.64s [%s] (%s / at lba %lld)\n", dmm.entries[i].name, file_type_to_str(dmm.entries[i].type), perms, uint128_to_u64(dmm.entries[i].lba));
        }
        dmm_lba = uint128_to_u64(dmm.extender_lba);
    }
    return PBFS_RES_SUCCESS;
}

// Test and print Disk Info
int pbfs_test(struct pbfs_mount* mnt, FILE* f, uint8_t debug) {
    PBFS_Header* hdr = (PBFS_Header*)calloc(1, sizeof(PBFS_Header));
    if (!hdr) {
        perror("Failed to allocate memory!\n");
        return AllocFailed;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    if (size < 512 + sizeof(PBFS_Header)) {
        fprintf(stderr, "Invalid Disk!\n");
        free(hdr);
        return InvalidHeader;
    }

    fseek(f, PBFS_HDR_START_BYTE, SEEK_SET);
    fread(hdr, sizeof(PBFS_Header), 1, f);
    if (pbfs_verify_header(hdr) != PBFS_RES_SUCCESS) {
        fprintf(stderr, "Invalid Header!\n");
        free(hdr);
        return InvalidHeader;
    }

    if (debug == 1) printf(
        "=== Header Information ===\n"
        "Magic: %.6s\n"
        "Block Size: %d\nTotal Blocks: %lld\n"
        "Disk Name: %s\nVolume ID: %lld\n"
        "DMM Root LBA: %lld\nBitmap Root LBA: %lld\n"
        "Kernel Table Root LBA: %lld\nBoot Partition LBA: %lld\n"
        "Data LBA: %lld\n",
        hdr->magic,
        (unsigned int)hdr->block_size, (unsigned long long int)uint128_to_u64(hdr->total_blocks),
        (char*)hdr->disk_name, (unsigned long long int)uint128_to_u64(hdr->volume_id),
        hdr->dmm_root_lba, hdr->bitmap_lba, hdr->kernel_table_lba, hdr->boot_partition_lba, hdr->data_start_lba
    );

    if (hdr->bitmap_lba <= 1) {
        fprintf(stderr, "Error: Bitmap LBA INVALID\n");
        free(hdr);
        return InvalidHeader;
    } else if (hdr->sysinfo_lba <= 1) {
        fprintf(stderr, "Error: SystemInfo LBA INVALID\n");
        free(hdr);
        return InvalidHeader;
    } else if (hdr->dmm_root_lba <= 1) {
        fprintf(stderr, "Error: DMM LBA INVALID\n");
        free(hdr);
        return InvalidHeader;
    } else if (hdr->data_start_lba <= 1) {
        fprintf(stderr, "Error: Data LBA INVALID\n");
        free(hdr);
        return InvalidHeader;
    }

    uint8_t* disk = (uint8_t*)calloc(1, hdr->block_size);
    if (!disk) {
        perror("Failed to allocate memory!\n");
        free(hdr);
        return AllocFailed;
    }

    if (debug == 1) {
        if (hdr->kernel_table_lba < 1) {
            printf("Info: No Kernel Table Found\n");
        } else {
            uint64_t kto = hdr->kernel_table_lba;
            PBFS_Kernel_Table* kernel_table = (PBFS_Kernel_Table*)disk;
            uint128_t test_lba = uint128_from_u32(1);

            printf("\n=== Kernel Table ===\n");
            while (kto > 1) {
                fseek(f, kto * hdr->block_size, SEEK_SET);
                fread(disk, hdr->block_size, 1, f);

                for (int i = 0; i < PBFS_KERNEL_TABLE_ENTRIES; i++) {
                    PBFS_Kernel_Entry* entry = &kernel_table->entries[i];
                    if (UINT128_NEQ(entry->lba, test_lba))
                        continue;
                    printf(
                        "== Kernel Entry (%d) ==\n"
                        "\tName: %.64s"
                        "\tLBA: %lld\n"
                        "\tCount: %lld\n",
                        i, entry->name,
                        (unsigned long long int)uint128_to_u64(entry->lba),
                        (unsigned long long int)uint128_to_u64(entry->count)
                    );
                }

                kto = uint128_to_u64(kernel_table->extender_lba);
            }
        }

        printf("=== Data ===\n");
        pbfs_list_dir(mnt, "/");
        // FILE* f = fopen(image, "rb");
        // if (!f) {
        //     perror("Failed to reopen image!\n");
        //     free(hdr);
        //     free(disk);
        //     return FileError;
        // }
        free(hdr);
        free(disk);
        return EXIT_SUCCESS;
    }

    if (debug) printf("Success!\n");

    free(hdr);
    free(disk);
    return EXIT_SUCCESS;
}

static int read_blk(struct block_device* dev, uint64_t lba, void* out) {
    fseek((FILE*)dev->driver_data, lba * dev->block_size, SEEK_SET);
    fread(out, dev->block_size, 1, (FILE*)dev->driver_data);
    return 1;
}

static int read_ex(struct block_device* dev, uint64_t lba, uint64_t count, void* out) {
    fseek((FILE*)dev->driver_data, lba * dev->block_size, SEEK_SET);
    fread(out, dev->block_size, count, (FILE*)dev->driver_data);
    return 1;
}

static int write_blk(struct block_device* dev, uint64_t lba, const void* buf) {
    fseek((FILE*)dev->driver_data, lba * dev->block_size, SEEK_SET);
    fwrite(buf, dev->block_size, 1, (FILE*)dev->driver_data);
    return 1;
}

static int write_ex(struct block_device* dev, uint64_t lba, uint64_t count, const void* buf) {
    fseek((FILE*)dev->driver_data, lba * dev->block_size, SEEK_SET);
    fwrite(buf, dev->block_size, count, (FILE*)dev->driver_data);
    return 1;
}

static int flush_ex(struct block_device* dev) {
    fflush((FILE*)dev->driver_data);
    return 1;
}

// Main function
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pbfs-cli <image> <[commands]>\nCommands:\n\t-bs/--block_size: Define the Block Size\n\t-tb/--total_blocks: Define Total number of blocks\n\t-dn/--disk_name: Define the disk name\n\t... USE -h/--help for more info.\n");
        return InvalidUsage;
    }

    uint8_t bootpart = 0;
    int bootpartsize = 0;
    uint8_t kerneltable = 0;

    uint64_t volume_id = 0;
    char disk_name[32];

    char name[64];
    uint32_t gid = 0;
    uint32_t uid = 0;
    char perms[6];
    char type[6];

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      printf(PBFS_CLI_HELP);
      return EXIT_SUCCESS;
    }

    char* image = argv[1];

    FILE* fp = NULL;

    if (access(image, F_OK) == 0) {
        fp = fopen(image, "rb+");
        if (!fp) {
            perror("Failed to open image!\n");
            return PBFS_ERR_UNKNOWN;
        }
    } else {
        printf("Image doesn't exist, creating...\n");
        fp = fopen(image, "wb+");
        if (!fp) {
            perror("Failed to open image!\n");
            return PBFS_ERR_UNKNOWN;
        }
    }

    struct pbfs_mount mnt = {0};
    struct block_device block_dev = {
        .block_size = 512,
        .block_count = 2048,
        .name = disk_name,
        .driver_data = (void*)fp,
        .read_block = read_blk,
        .read = read_ex,
        .write_block = write_blk,
        .write = write_ex,
        .flush = flush_ex
    };
    struct pbfs_funcs funcs = {
        .free=free,
        .malloc=malloc
    };

    pbfs_init(&funcs);

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-bs") == 0 || strcmp(argv[i], "--block_size") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <size>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            
            block_dev.block_size = (uint32_t)atoi(argv[i + 1]);
            if (block_dev.block_size < 512) {
                fprintf(stderr, "Block Size cannot be lower than 512!\n");
                fclose(fp);
                return InvalidUsage;
            }
            i++;
        } else if (strcmp(argv[i], "-tb") == 0 || strcmp(argv[i], "--total_blocks") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <count>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            
            block_dev.block_count = (uint64_t)atoll(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-dn") == 0 || strcmp(argv[i], "--disk_name") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <name>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            
            strncpy(disk_name, argv[i + 1], 31);
            disk_name[31] = '\0';
            i++;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--volume_id") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <volume_id>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            volume_id = (uint64_t)atoll(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            printf("Formating Image...\n");
            printf(
                "Block Size: %u\nTotal Blocks: %llu\nDisk Name: %s\nReserve Kernel Table: %s\nBoot Parition Size: %u\n",
                block_dev.block_size, block_dev.block_count, block_dev.name, kerneltable == 1 ? "True" : "False", bootpart == 1 ? bootpartsize : 0
            );
            int out = pbfs_format(&block_dev, kerneltable, bootpartsize, volume_id);

            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(PBFS_CLI_HELP);
            fclose(fp);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--create") == 0) {
            printf("Creating Image...\n");
            char* disk = (char*)malloc(block_dev.block_size);
            if (!disk) {
                perror("Allocation Failed!\n");
                fclose(fp);
                return AllocFailed;
            }
            memset(disk, 0, block_dev.block_size);
            fseek(fp, 0, SEEK_SET);
            for (uint64_t i = 0; i < block_dev.block_count; i++) {
                fwrite(disk, block_dev.block_size, 1, fp);
            }
            fseek(fp, 0, SEEK_SET);
            free(disk);
            printf("Done!\n");
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--add") == 0) {
            // Add a file
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }

            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            if (strlen(filepath) < 1 || strlen(name) < 1) {
                strcpy(name, filepath);
            }
            printf("Adding File [%s] to Image...\n", name);
            FILE* f = fopen(filepath, "rb");
            if (!f) {
                perror("Failed to open file!\n");
                fclose(fp);
                return PBFS_ERR_UNKNOWN;
            }
            fseek(f, 0, SEEK_END);
            uint64_t data_size = ftell(f);
            rewind(f);
            uint8_t* data = (uint8_t*)malloc(data_size);
            if (!data) {
                perror("Failed to allocate memory!\n");
                fclose(fp);
                fclose(f);
                return PBFS_ERR_UNKNOWN;
            }
            fread(data, 1, data_size, f);
            fclose(f);

            int out = pbfs_add(&mnt, name, uid, gid, parse_file_type(type), parse_file_perms(perms), data, data_size);
            free(data);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--remove") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }

            // Removes a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            if (strlen(name) < 1) {
                strcpy(name, filepath);
            }
            printf("Removing File [%s] from Image...\n", name);
            int out = pbfs_remove(&mnt, name);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--update") == 0) {
            // Add a file
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }

            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            if (strlen(filepath) < 1 || strlen(name) < 1) {
                fclose(fp);
                fprintf(stderr, "No path specified, use --name!\n");
                return InvalidArgument;
            }
            printf("Updating File [%s] to [%s] in Image...\n", name, filepath);
            FILE* f = fopen(filepath, "rb");
            if (!f) {
                perror("Failed to open file!\n");
                fclose(fp);
                return PBFS_ERR_UNKNOWN;
            }
            fseek(f, 0, SEEK_END);
            uint64_t data_size = ftell(f);
            rewind(f);
            uint8_t* data = (uint8_t*)malloc(data_size);
            if (!data) {
                perror("Failed to allocate memory!\n");
                fclose(fp);
                fclose(f);
                return PBFS_ERR_UNKNOWN;
            }
            fread(data, 1, data_size, f);
            fclose(f);

            int out = pbfs_update_file(&mnt, name, data, data_size);
            free(data);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-ad") == 0 || strcmp(argv[i], "--add_dir") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            // Add a dir
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            printf("Adding Dir [%s] to Image...\n", filepath);
            int out = pbfs_add_dir(&mnt, filepath, uid, gid, parse_file_perms(perms));
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            // List a path
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            printf("Listing [%s] from Image...\n", filepath);
            int out = pbfs_list_dir(&mnt, filepath);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-rf") == 0 || strcmp(argv[i], "--read_file") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            // Read a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            printf("Reading [%s] from Image...\n", filepath);
            uint8_t* data = NULL;
            size_t data_size = 0;
            int out = pbfs_read_file(&mnt, filepath, &data, &data_size);
            if (out != PBFS_RES_SUCCESS) {
                if (data) free(data);
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            for (size_t i = 0; i < data_size; i++) {
                if (data[i] == '\0') break;
                printf("%c", (char)data[i]);
            }
            free(data);
            printf("\nDone!\n");
        } else if (strcmp(argv[i], "-rfb") == 0 || strcmp(argv[i], "--read_file_binary") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            // Read a file
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            printf("Reading [%s] from Image...\n", filepath);
            uint8_t* data = NULL;
            size_t data_size = 0;
            int out = pbfs_read_file(&mnt, filepath, &data, &data_size);
            if (out != PBFS_RES_SUCCESS) {
                if (data) free(data);
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
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
        } else if (strcmp(argv[i], "--gid") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <gid>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            gid = (uint32_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--uid") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <uid>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            uid = (uint32_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--name") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <name>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            strncpy(name, argv[i + 1], sizeof(name));
            i++;
        } else if (strcmp(argv[i], "--permissions") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <permissions>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            strncpy(perms, argv[i + 1], sizeof(perms));
            i++;
        } else if (strcmp(argv[i], "--type") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <type>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            strncpy(type, argv[i + 1], sizeof(type));
            i++;
        } else if (strcmp(argv[i], "-btl") == 0 || strcmp(argv[i], "--bootloader") == 0) {
            // Add a bootloader
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <filepath>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-btp") == 0 || strcmp(argv[i], "--boot_partition") == 0) {
            if (i + 2 > argc) {
                printf("Usage: pbfs-cli <image> %s <blocks>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            bootpartsize = (int)atoi(argv[i + 1]);
            bootpart = 1;
            i++;
        } else if (strcmp(argv[i], "-rkt") == 0 || strcmp(argv[i], "--reserve_kernel_table") == 0) {
            kerneltable = 1;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            int out = pbfs_test(&mnt, fp, 1);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
        } else if (strcmp(argv[i], "-chp") == 0 || strcmp(argv[i], "--change_permissions") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 2 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <path> --permissions <perms>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            if (strlen(filepath) < 1 || strlen(perms) < 1) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <path> --permissions <perms>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            printf("Changing permissions [%s] for [%s]...\n", perms, filepath);
            int out = pbfs_change_permissions(&mnt, filepath, parse_file_perms(perms));
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-cpy") == 0 || strcmp(argv[i], "--copy") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 2 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <from path> <to path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            char* to_path = argv[i + 1];
            i++;
            printf("Copying [%s] -> [%s]...\n", filepath, to_path);
            int out = pbfs_copy(&mnt, filepath, to_path);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-mv") == 0 || strcmp(argv[i], "--move") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 3 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <from path> <to path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            char* to_path = argv[i + 1];
            i++;
            printf("Moving [%s] -> [%s]...\n", filepath, to_path);
            int out = pbfs_move(&mnt, filepath, to_path);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-rn") == 0 || strcmp(argv[i], "--rename") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 3 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <from path> <new name>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            char* new_name = argv[i + 1];
            i++;
            printf("Renaming [%s] -> [%s]...\n", filepath, new_name);
            int out = pbfs_rename(&mnt, filepath, new_name);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-rk") == 0 || strcmp(argv[i], "--remove_kernel") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 2 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            printf("Removing Kernel [%s]...\n", filepath);
            int out = pbfs_remove_kernel(&mnt, filepath);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                fclose(fp);
                return out;
            }
            printf("Done!\n");
        } else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kernel") == 0) {
            if (mnt.active != true) {
                if (pbfs_mount(&block_dev, &mnt) != PBFS_RES_SUCCESS) {
                    fprintf(stderr, "Failed to mount!\n");
                    fclose(fp);
                    fclose(fp);
                    return PBFS_ERR_UNKNOWN;
                }
            }
            
            if (i + 3 > argc) {
                fprintf(stderr, "Usage: pbfs-cli <image> %s <file> <path>\n", argv[i]);
                fclose(fp);
                return InvalidUsage;
            }
            char* filepath = argv[i + 1];
            i++;
            char* name_path = argv[i + 2];
            i++;
            printf("Adding Kernel [%s]...\n", name_path);

            FILE* f = fopen(filepath, "rb");
            if (!f) {
                perror("Failed to open file!\n");
                fclose(fp);
                return PBFS_ERR_UNKNOWN;
            }
            fseek(f, 0, SEEK_END);
            uint64_t data_size = ftell(f);
            rewind(f);
            uint8_t* data = (uint8_t*)malloc(data_size);
            if (!data) {
                perror("Failed to allocate memory!\n");
                fclose(fp);
                fclose(f);
                return PBFS_ERR_UNKNOWN;
            }
            fread(data, 1, data_size, f);
            fclose(f);

            int out = pbfs_add_kernel(&mnt, filepath, data, data_size);
            if (out != PBFS_RES_SUCCESS) {
                fprintf(stderr, "An Error Occurred!\n");
                free(data);
                fclose(fp);
                return out;
            }
            free(data);
            printf("Done!\n");
            i++; // skip new_name
        } else {
            printf("Unknown command: %s\n\tTry using -h/--help for more info!\n", argv[i]);
            fclose(fp);
            return InvalidArgument;
        }
    }

    fflush(fp);
    fclose(fp);
    return EXIT_SUCCESS;
}
