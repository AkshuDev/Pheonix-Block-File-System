# Variables
BuildDir := ../build-fs
ObjDir := $(BuildDir)/objs

pbfs_main_fs := pbfs.c
pbfs_main_fs_out := $(BuildDir)/pbfs.o

HeadersDir := ../headers

PefiDir := ../PEFI
PefiHeaderDir := $(PefiDir)/includes
PefiLibDir := $(PefiDir)/build

tool_disk_io_o := $(ObjDir)/disk_io_pm.o
tool_disk_params_o := $(ObjDir)/disk_params_pm.o

tool_disk_io := ../tools/disk_io_pm.asm
tool_disk_params := ../tools/disk_params_pm.asm

OBJS := $(tool_disk_io_o) $(tool_disk_params_o)

CC := gcc
CFLAGS := -ffreestanding -nostartfiles -fno-stack-protector -fno-pic -nostdlib -I$(HeadersDir) -I$(PefiHeaderDir) -L$(PefiLibDir) -lpefi

LD := ld
LDFLAGS := -nostdlib -m32 -shared

ASM := nasm
ASMFLAGS := -f elf32

AR := ar


TARGET := $(BuildDir)/pbfs.a

# === TARGET ===
.PHONY: all clean

all: $(BuildDir) $(ObjDir) $(TARGET)

# Make the dirs
$(BuildDir):
	mkdir -p $(BuildDir)

$(ObjDir):
	mkdir -p $(ObjDir)

# Assemble
$(tool_disk_io_o): $(tool_disk_io)
	@echo "Creating Assembly files..."
	$(ASM) $(ASMFLAGS) $(tool_disk_io) -o $(tool_disk_io_o)

$(tool_disk_params_o): $(tool_disk_params)
	$(ASM) $(ASMFLAGS) $< -o $@

$(pbfs_main_fs_out): $(pbfs_main_fs)
	$(CC) $(CFLAGS) $< -o $@

# Link and build
$(TARGET): $(OBJS)
	$(AR) rsc $(TARGET) $(OBJS)

# Clean
clean:
	rm -rf $(BuildDir) $(TARGET)
