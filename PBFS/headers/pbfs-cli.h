#pragma once

#ifdef _WIN32
    #define CALLCONV __cdecl
#else
    #define CALLCONV
#endif

#define PBFS_CLI_HELP "Usage: pbfs-cli <image> <commands>\nCommands:\n\t-bs <size>: Define the Block Size\n\t-tb <count>: Define Total number of blocks\n\t-dn <name>: Define the disk name\n\t-f: Format the disk\n\t-c: Create the disk\n\t-add <filename> <filepath>: Adds a file to the image.\n\t-bootloader <filepath>: Adds a bootloader to the image (MBR).\n\t-partboot <Number of Blocks>: Adds a Boot partition of size Block Size times Span specified here.\n\t-partkernel: Reserves a Kernel Entry block\n\t-kernel <path>: Adds a kernel with no metadata and adds it to the kernel entry if found.\n"