# PBFS - Pheonix Block File System / Python-Based File System

> A future-proof, ultra-large-scale block-level file system designed for modern and forward-compatible computing needs.

---

## Why Two Names?

PBFS is referred to as:

- **Pheonix Block File System**: When discussing block-level storage, structure, and system integration. This is the PBFS in this github repo.
- **Python-Based File System**: When focusing on its first implementation, which is purely in Python. It is the first PBFS that is created purely in python.

Both are valid and describe different aspects of the same system.

---

## Location in the `phardwareitk` Python Module

PBFS is implemented as part of the [`phardwareitk`](https://pypi.org/project/phardwareitk) module.

It is located under the submodule `PENV`.

---

The python version can be installed by doing this command -

`bash`
```bash
pip install phardwareitk
```
`powershell`
```powershell
pip install phardwareitk
```
`cmd`
```cmd
pip install phardwareitk
```

# PBFS (Pheonix-Block-File-System)
PBFS is a 128-bit addressable file system, which allows it to support upto 340 million million yottabytes or 340 undecillion bytes, compared to normal 64-bit support of other file systems, upto 16 exabytes.

PBFS offers multiple inbuilt structures to help and support users (The program using PBFS) -
## Kernel Table
It is an extendable table of various OS/Kernel/Bootloader/etc entries, can be used in multiple cases.

## Sysinfo
An reserved block for SystemInfo without extensive searching to improve performance

## Bootloader Partition
Multiple reserved blocks for bootloaders which can be added via PBFS CLI

## Inbuilt Permissions
PBFS has inbuilt permission handling for ease-of-use
Some permissions are -
1. Read
2. Write
3. Execute
4. Locked (Cannot be deleted)
5. Protected (No Read/Write/Execute, and Locked)

And more

## Inbuilt Metadata and File types
PBDS also has inbuilt metdata and file types such as -
1. File
2. Directory
3. System File
4. Symbolic Link
5. PBFS File

And more

## Various easy to use functions
PBFS offers multiple functions like chp (Change Permissions), remove, copy, move, add (Multi type support except dir), add_dir, list_kernels, add_kernel, remove_kernel, add_bootloader, and more....

## Any environment support
PBFS libraries are designed to be able to run in any environment, for example -

1. [AOS++](https://github.com/AkshuDev/AOS-1.0) uses PBFS in an Freestanding/Kernel Environment.
2. PBFS Cli itself uses PBFS libraries in an Linux/Windows/etc Environment

# PBFS CLI
Pheonix-Block-File-System Command-Line-Interface is an program/utility to use PBFS accessable to users using commands.

It supports multiple commands such as -

1. **`-bs / --block_size <size>`**
   Define the block size *(Format only)*

2. **`-tb / --total_blocks <count>`**
   Define total number of blocks *(Format only)*

3. **`-dn / --disk_name <name>`**
   Define the disk name *(Format only)*

4. **`-v / --volume_id <id>`**
   Set the volume ID *(Format only)*

5. **`--gid <id>`**
   Set GID for added file/directory *(Add only)*

6. **`--uid <id>`**
   Set UID for added file/directory *(Add only)*

7. **`--name <name>`**
   Set name for file *(Add / Update only)*

8. **`--permissions <permissions>`**
   Set file permissions *(Add / Change only)*

   * `r` → Read
   * `w` → Write
   * `e` → Execute
   * `s` → System
   * `p` → Protected
   * `l` → Locked
   * Examples: `rw`, `res`

9. **`--type <type>`**
   Set file type *(Add file only)*

   * `dir` → Directory
   * `file` → File
   * `res` → Reserved
   * `sys` → System

10. **`-f / --format`**
    Format the disk

11. **`-c / --create`**
    Create the disk

12. **`-a / --add <filepath>`**
    Add file to image *(uses --name or filepath)*

13. **`-ad / --add_dir <path>`**
    Add directory to image

14. **`-cpy / --copy <from> <to>`**
    Copy file within image

15. **`-rn / --rename <from> <to>`**
    Rename file *(acts like move)*

16. **`-chp / --change_permissions <path>`**
    Change file permissions *(use --permissions)*

17. **`-mv / --move <from> <to>`**
    Move file within image

18. **`-r / --remove <file>`**
    Remove file *(uses --name or filepath)*

19. **`-u / --update <filepath>`**
    Update file *(uses --name or filepath)*

20. **`-btl / --bootloader <filepath>`**
    Add bootloader to image

21. **`-rbp / --reserve_boot_partition <start_lba> <blocks>`**
    Reserve boot partition

    * Start LBA must be aligned to 4096
    * Max blocks: 511

22. **`-rkt / --reserve_kernel_table`**
    Reserve kernel table block *(extendable)*

23. **`-k / --kernel <path>`**
    Add kernel to kernel table

24. **`-rk / --remove_kernel <path>`**
    Remove kernel from kernel table

25. **`-t / --test`**
    Test disk and display info

26. **`-l / --list <path>`**
    List directory or show file

27. **`-rf / --read_file <path>`**
    Read file from image

28. **`-rfb / --read_file_binary <path>`**
    Read file in binary format

29. **`-gpt / --gpt`**
    Add GPT headers

    * Requires bootloader partition ≥ 30 blocks

30. **`-mbr / --mbr`**
    Add MBR headers

    * Requires bootloader partition ≥ 2 blocks

31. **`-h / --help`**
    Show help message


