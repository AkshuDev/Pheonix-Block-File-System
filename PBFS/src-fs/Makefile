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

# === Main Stuff ===
BASE_DIR := ..

SRC_DIR := .
TOOLS_DIR := $(BASE_DIR)/tools

HEADER_DIR := $(BASE_DIR)/headers

BIN_DIR := $(BASE_DIR)/build-fs
BUILD_DIR := $(BIN_DIR)/objs

SRCS := $(SRC_DIR)/pbfs-fs.c

SRCS_UTILS := $(BASE_DIR)/tools/utils.c $(BASE_DIR)/tools/xxhash3_128b.c
OBJS_UTILS := $(patsubst $(TOOLS_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_UTILS))

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

CC := gcc
AR := ar

EXTRA ?= 
OPT_LEVEL ?= 3
CFLAGS := $(EXTRA) -O$(OPT_LEVEL) -I $(HEADER_DIR) -fno-stack-protector -Wall -Wextra -Werror=implicit-function-declaration -std=gnu11 -pipe -ffreestanding
ARFLAGS := rcs

LIB := $(BIN_DIR)/libpbfs.a

.PHONY: all clean

$(LIB): $(OBJS) $(OBJS_UTILS)
	@printf "$(CLR_YELLOW)-- Creating PBFS Library...$(CLR_RESET)\n"

	@mkdir -p $(BIN_DIR)
	@$(AR) $(ARFLAGS) $@ $^

	@printf "$(CLR_GREEN)-- Created at \"$@\"$(CLR_RESET)\n"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "$(CLR_YELLOW)---- Compiling Source: \"$<\"$(CLR_RESET)\n"

	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

	@printf "$(CLR_GREEN)---- Compiled to \"$@\"$(CLR_RESET)\n"

$(BUILD_DIR)/%.o: $(TOOLS_DIR)/%.c
	@printf "$(CLR_YELLOW)---- Compiling Tool: \"$<\"$(CLR_RESET)\n"

	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

	@printf "$(CLR_GREEN)---- Compiled to \"$@\"$(CLR_RESET)\n"

clean:
	@printf "$(CLR_RED)-- Cleaning...$(CLR_RESET)\n"
	@rm -rf $(BUILD_DIR) $(LIB)
	@printf "$(CLR_GREEN)-- Cleaned$(CLR_RESET)\n"

all: $(LIB)
