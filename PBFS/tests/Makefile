# Use the mingw-w64 cross-compiler for PE/COFF output
CC = x86_64-w64-mingw32-gcc
LD = x86_64-w64-mingw32-ld
MFORMAT = mformat
MCOPY = mcopy
PBFS_CLI = ../build-cli/pbfs-cli

# Paths
PEFI_DIR = ../PEFI
INC = -I $(PEFI_DIR)/includes -I ../headers

CFLAGS = $(INC) -ffreestanding -fno-stack-protector -mno-red-zone -Wall -Wextra -std=c11

LDFLAGS = -Wl,--subsystem,10 -e efi_main -nostdlib -L $(PEFI_DIR)/build
LDLIBS = -lpefi

TARGET = ./objs/test_bootx64.efi
DISK_IMG = ./objs/test_disk.img
PBFS_IMG = ./bin/test_disk.pbfs
SRCS = test_bootx64.c
OBJS = $(patsubst %.c,./objs/%.o,$(SRCS))

all: $(PBFS_IMG)

$(PBFS_IMG): $(DISK_IMG)
	@echo "Making $(PBFS_IMG)"
	$(PBFS_CLI) $(PBFS_IMG) -bs 512 -tb 2048 -dn TestDisk -v 1 -c -rbp 1024 1024 -f -gpt -btl $(DISK_IMG)
	@echo "Made $(PBFS_IMG)"

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

./objs/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DISK_IMG): $(TARGET)
	@echo "Creating 1MB FAT32 payload..."
	dd if=/dev/zero of=$(DISK_IMG) bs=1k count=1024
	$(MFORMAT) -i $(DISK_IMG) -v "ESP" -C -t 80 -h 2 -s 18 ::
	mmd -i $(DISK_IMG) ::/EFI
	mmd -i $(DISK_IMG) ::/EFI/BOOT
	$(MCOPY) -i $(DISK_IMG) $(TARGET) ::/EFI/BOOT/BOOTX64.EFI
	@echo "Done. Payload is in $(DISK_IMG)"

clean:
	rm -f $(OBJS) $(TARGET) $(DISK_IMG) $(PBFS_IMG)

.PHONY: all clean