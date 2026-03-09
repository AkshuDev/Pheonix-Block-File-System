# Pheonix Extensible Firmware Interface Driver

# === Directories ===
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := includes

# === Files ===
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CFLAGS := -DBUILD_LIB -m64 -w -c -ffreestanding -fno-stack-protector -mno-red-zone -Wall -Wextra -std=c11 -I$(INCLUDE_DIR)
AR := ar
CC := x86_64-w64-mingw32-gcc

TARGET := $(BUILD_DIR)/libpefi.a

# === Rules ===
.PHONY: all clean

all: $(TARGET)

# Build targets
# Build Lib
$(TARGET): $(OBJ_FILES)
	$(AR) rcs $@ $(OBJ_FILES)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# === End ===
