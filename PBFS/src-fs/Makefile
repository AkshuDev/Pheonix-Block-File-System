BASE_DIR := ..

SRC_DIR := .
TOOLS_DIR := $(BASE_DIR)/tools

HEADER_DIR := $(BASE_DIR)/headers

BIN_DIR := $(BASE_DIR)/build-fs
BUILD_DIR := $(BIN_DIR)/objs

SRCS_WDRIVER := 
SRCS_NDRIVER := $(SRC_DIR)/pbfs-fs-ndrivers.c

SRCS_UTILS := $(BASE_DIR)/tools/utils.c
OBJS_UTILS := $(patsubst $(TOOLS_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_UTILS))

OBJS_WDRIVER := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_WDRIVER))
OBJS_NDRIVER := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS_NDRIVER))

CC := gcc
AR := ar

CFLAGS := -I $(HEADER_DIR) -fno-stack-protector -Wall -Wextra -Werror=implicit-function-declaration -std=gnu11 -pipe -ffreestanding
ARFLAGS := rcs

LIB_NDRIVERS := $(BIN_DIR)/libpbfs-ndrivers.a
LIB_WDRIVERS := $(BIN_DIR)/libpbfs-wdrivers.a

.PHONY: all clean wdrivers ndrivers

$(LIB_NDRIVERS): $(OBJS_NDRIVER) $(OBJS_UTILS)
	@mkdir -p $(BIN_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIB_WDRIVERS): $(OBJS_WDRIVER) $(OBJS_UTILS)
	@mkdir -p $(BIN_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TOOLS_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB_WDRIVERS) $(LIB_NDRIVERS)

wdrivers: $(LIB_WDRIVERS)
ndrivers: $(LIB_NDRIVERS)
all: $(LIB_NDRIVERS)