# Pheonix Extensible Firmware Interface Driver

# === Directories ===
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := includes

# === Platform detection ===
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    TARGET := $(BUILD_DIR)/pefi.dll
    DLLFLAGS := -shared -Wl,--out-implib,$(BUILD_DIR)/libpefi.a
else
    TARGET := $(BUILD_DIR)/libpefi.a
endif

# === Files ===
SRC_FILES := $(SRC_DIR)/main.c \
             $(SRC_DIR)/efi_graphics.c \
             $(SRC_DIR)/efi_disk.c \
             $(SRC_DIR)/efi_memory.c \
             $(SRC_DIR)/efi_init.c

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CFLAGS := -ffreestanding -fpic -mno-red-zone -Wall -Wextra -nostdlib -m64 -I$(INCLUDE_DIR)
AR := ar
CC := gcc

# === Rules ===
.PHONY: all clean

all: $(TARGET)

# Compile .c -> .o into build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build targets
$(BUILD_DIR)/pefi.dll: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(DLLFLAGS) $^ -o $@

$(BUILD_DIR)/libpefi.a: $(OBJ_FILES)
	$(AR) rcs $@ $^

# Clean
clean:
	rm -rf $(BUILD_DIR)

# === End ===
