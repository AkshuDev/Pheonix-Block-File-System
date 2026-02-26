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
"\t--gid <id>: Sets the GID for the added file (ADD FILE Only)\n" \
"\t--uid <id>: Sets the UID for the added file (ADD FILE Only)\n" \
"\t--name <name>: Sets the name for the added file (ADD/UPDATE FILE Only)\n" \
"\t--permissions <permissions>: Sets the Permissions for the added file (ADD FILE Only). Format -\n" \
"\t\tr (read)/w (write)/e (execute)/s (system)/p (protected)/l (locked)\n" \
"\t\tCan add multiple together, examples - rw (read+write) / res (read + execute + system)\n" \
"\t--type <type>: Sets the Type for the added file (ADD FILE Only). Types -\n" \
"\t\tdir, file, res (reserve), sys (system)\n" \
"\t-f/--format: Format the disk\n" \
"\t-c/--create: Create the disk\n" \
"\t-a/--add <filepath>: Adds a file to the image. (use --name for the file name or file path will be used)\n" \
"\t-ad/--add_dir <path>: Adds a dir to the image at the specified path.\n" \
"\t-r/--remove <file>: Removes the provided file from the image. (use --name for the file name or file path will be used)\n" \
"\t-u/--update <filepath>: Updates a file on the image. (use --name for the file name or file path will be used)\n" \
"\t-btl/--bootloader <filepath>: Adds a bootloader to the image (MBR).\n" \
"\t-btp/--boot_partition <Number of Blocks>: Adds a Boot partition of specified Block Size.\n" \
"\t-rkt/--reserve_kernel_table: Reserves a Kernel Table Block (Extendable).\n" \
"\t-k/--kernel <path>: Adds a kernel with minimal metadata to the kernel table if found.\n" \
"\t-t/--test: Tests and shows information about the disk.\n" \
"\t-l/--list <path>: Lists a dir or shows file path if file exists.\n" \
"\t-rf/--read_file <path>: Reads a file from within the image.\n" \
"\t-rfb/--read_file_binary <path>: Reads a file from within the image. (binary)\n" \
"\t-h/--help: Shows this display message.\n"