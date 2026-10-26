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

SRCS_NDRIVER := $(SRC_DIR)/pbfs-fs.c

SRCS_UTILS := $(BASE_DIR)/tools/utils.c $(BASE_DIR)/tools/xxhash3_128b.c
OBJS_UTILS := $(patsubst $(TOOLS_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_UTILS))

OBJS_NDRIVER := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_NDRIVER))

CC := gcc
AR := ar

EXTRA ?= 
OPT_LEVEL ?= 3
CFLAGS := $(EXTRA) -O$(OPT_LEVEL) -I $(HEADER_DIR) -fno-stack-protector -Wall -Wextra -Werror=implicit-function-declaration -std=gnu11 -pipe -ffreestanding
ARFLAGS := rcs

LIB_NDRIVERS := $(BIN_DIR)/libpbfs-ndrivers.a

.PHONY: all clean ndrivers

$(LIB_NDRIVERS): $(OBJS_NDRIVER) $(OBJS_UTILS)
	@printf "$(CLR_YELLOW)-- Creating NDriver Library...$(CLR_RESET)\n"

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
	@rm -rf $(BUILD_DIR) $(LIB_NDRIVERS)
	@printf "$(CLR_GREEN)-- Cleaned$(CLR_RESET)\n"

ndrivers: $(LIB_NDRIVERS)
all: $(LIB_NDRIVERS)