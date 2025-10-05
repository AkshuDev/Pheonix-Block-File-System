ASM := nasm
CC := gcc
LD := ld
OBJCOPY := objcopy
AR := ar

CFLAGS := -m32 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Wall -I ../../headers
LDFLAGS := -m elf_i386 -Ttext 0x1000

BUILD_DIR := build
BIN_DIR := bin

stage1 := $(BUILD_DIR)/boot.bin
stage2 := $(BUILD_DIR)/stage2.bin

DISK := $(BIN_DIR)/disk.pbfs
PBFS_BLT_STUB := ../../build-fs/libpbfs_btl_stub.a

PBFS_LIB := ../../build-fs/pbfs.a
PBFS_CLI := ../../build-cli/pbfs-cli

all: $(BUILD_DIR) $(BIN_DIR) $(DISK)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(stage1): src/stage1.asm
	$(ASM) -f bin $< -o $@

$(BUILD_DIR)/stage2.o: src/stage2.asm
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/stage2_main.o: src/stage2_main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(stage2): $(BUILD_DIR)/stage2.o $(BUILD_DIR)/stage2_main.o $(PBFS_BLT_STUB)
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/main.elf $^
	$(OBJCOPY) -O binary $(BUILD_DIR)/main.elf $@

$(DISK): $(stage1) $(stage2)
	$(PBFS_CLI) $(DISK) -bs 512 -tb 4096 -dn TestDisk -f #format disk
	dd if=$(stage1) of=$(DISK) bs=512 count=1 conv=notrunc
	dd if=$(stage2) of=$(DISK) bs=512 seek=4 conv=notrunc

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
