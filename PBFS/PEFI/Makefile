# Pheonix Extensible Firmware Interface Driver

# === Colors ===
CLR_RESET   := \033[0m

CLR_BLACK   := \033[30m
CLR_RED     := \033[31m
CLR_GREEN   := \033[32m
CLR_YELLOW  := \033[33m
CLR_BLUE    := \033[34m
CLR_MAGENTA := \033[35m
CLR_CYAN    := \033[36m
CLR_WHITE   := \033[37m

# === Directories ===
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := includes

# === Files ===
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

CFLAGS := -DBUILD_LIB -m64 -mabi=ms -w -c -ffreestanding -fno-stack-protector -mno-red-zone -Wall -Wextra -std=c11 -I$(INCLUDE_DIR)
AR := ar
CC := x86_64-w64-mingw32-gcc

TARGET := $(BUILD_DIR)/libpefi.a

# === Rules ===
.PHONY: all clean

all: $(TARGET)

# Build targets
# Build Lib
$(TARGET): $(OBJ_FILES)
	@printf "$(CLR_YELLOW)-- Creating Library...$(CLR_RESET)\n"
	@$(AR) rcs $@ $(OBJ_FILES)
	@printf "$(CLR_GREEN)-- Created Library at \"$@\"$(CLR_RESET)\n"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "$(CLR_YELLOW)---- Compiling \"$<\"...$(CLR_RESET)\n"
	
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) $< -o $@

	@printf "$(CLR_GREEN)---- Compiled to \"$@\"$(CLR_RESET)\n"

# Clean
clean:
	@printf "$(CLR_RED)-- Cleaning...$(CLR_RESET)\n"
	@rm -rf $(BUILD_DIR)
	@printf "$(CLR_GREEN)-- Cleaned$(CLR_RESET)\n"

# === End ===
