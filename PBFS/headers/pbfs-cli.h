#pragma once

#ifdef _WIN32
    #define CALLCONV __cdecl
#else
    #define CALLCONV
#endif

#define PBFS_CLI_HELP "Usage: pbfs-cli <image> <[commands]>\n" \
"Commands:\n" \
"\t-bs/--block_size <size>: Define the Block Size (Format only)\n" \
"\t-tb/--total_blocks <count>: Define Total number of blocks (Format Only)\n" \
"\t-dn/--disk_name <name>: Define the disk name (Format Only)\n" \
"\t-v/--volume_id <id>: Sets the volume id of disk (Format Only)\n" \
"\t-f/--format: Format the disk\n" \
"\t-c/--create: Create the disk\n" \
"\t-a/--add <filename> <filepath>: Adds a file to the image.\n" \
"\t-btl/--bootloader <filepath>: Adds a bootloader to the image (MBR).\n" \
"\t-btp/--boot_partition <Number of Blocks>: Adds a Boot partition of specified Block Size.\n" \
"\t-akt/--add_kernel_table: Reserves a Kernel Table Block (Extendable).\n" \
"\t-k/--kernel <path>: Adds a kernel with minimal metadata to the kernel table if found.\n" \
"\t-t/--test: Tests and shows information about the disk.\n" \
"\t-h/--help: Shows this display message.\n"