# Pheonix Extensible Firmware Interface Driver

# === Directories ===
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := includes

# === Files ===
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CFLAGS := -w -c -fpic -Wall -Wextra -I$(INCLUDE_DIR)
AR := ar
CC := gcc

ARCH := $(shell uname -m)

ifeq ($(ARCH), x86_64)
    CFLAGS += -m64 -mno-red-zone
endif

# === Platform ===

UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    TARGET := $(BUILD_DIR)/pefi.dll
    LIBFLAGS := -shared -Wl,--out-implib,libpefi.a
else
    TARGET := $(BUILD_DIR)/libpefi.so
    LIBFLAGS := -shared
    CFLAGS += -fno-stack-protector
endif


# === Rules ===
.PHONY: all clean

all: $(TARGET)

# Build targets
# Build Lib
$(TARGET): $(OBJ_FILES)
	$(CC) $(LIBFLAGS) -o $@ $(OBJ_FILES)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# === End ===
