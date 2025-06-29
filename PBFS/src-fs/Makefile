# Variables
Cdir := $(shell pwd)
BuildDir := $(Cdir)/build-fs
pbfs_main_fs := $(Cdir)/src-fs/pbfs.c
pbfs_main_fs_out := $(BuildDir)/pbfs.o

HeadersDir := $(Cdir)/headers

PefiDir := $(Cdir)/PEFI
PefiHeaderDir := $(PefiDir)/includes
PefiLibDir := $(PefiDir)/build

tool_disk_io_o := $(Cdir)/build-fs/disk_io.o
tool_disk_params_o := $(Cdir)/build-fs/disk_params.o

tool_disk_io := $(Cdir)/tools/disk_io.asm
tool_disk_params := $(Cdir)/tools/disk_params.asm

OBJS := $(tool_disk_io_o) $(tool_disk_params_o)

CC := gcc
CFLAGS := -ffreestanding -nostartfiles -fno-pic -nostdlib -I$(HeadersDir) -I$(PefiHeaderDir) -L$(PefiLibDir) -lpefi

LD := ld
LDFLAGS := -nostdlib

ASM := nasm
ASMFLAGS := -f win64


TARGET := $(BuildDir)/pbfs

# === TARGET ===
.PHONY: all clean

all: $(BuildDir) $(TARGET)

# Make the dirs
$(BuildDir):
	mkdir -p $(BuildDir)

# Assemble
$(tool_disk_io_o): $(tool_disk_io)
	$(ASM) $(ASMFLAGS) $< -o $@

$(tool_disk_params_o): $(tool_disk_params)
	$(ASM) $(ASMFLAGS) $< -o $@

$(pbfs_main_fs_out): $(pbfs_main_fs)
	$(CC) $(CFLAGS) $< -o $@

# Link and build
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

# Clean
clean:
	rm -rf $(BuildDir) $(TARGET)