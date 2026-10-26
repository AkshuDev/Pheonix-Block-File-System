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

# === CONFIG ===
BaseDir := ..

build_dir := $(BaseDir)/build-cli
obj_dir = $(build_dir)/objs
headers_dir := $(BaseDir)/headers

C_SRCFILES = $(wildcard *.c) $(BaseDir)/tools/utils.c $(BaseDir)/tools/xxhash3_128b.c $(BaseDir)/src-fs/pbfs-fs.c
H_SRCFILES = $(wildcard $(headers_dir)/*.h)

C_OBJS = $(patsubst %.c, $(build_dir)/%.o, $(C_SRCFILES))
OBJS := $(C_OBJS) $(ASM_OBJS)

# === END CONFIG ===
# === TOOLS ===
CC := gcc

EXTRA ?= 
OPT_LEVEL ?= 3

CFLAGS := $(EXTRA) -O$(OPT_LEVEL) -I$(headers_dir) -Wall -Wextra
CFLAGS_LIB := 

OS := $(shell uname -s)

ifeq ($(OS), Windows_NT)
	ASMFLAGS += win32
else ifeq ($(findstring MSYS_NT, $(OS)), MSYS_NT)
	ASMFLAGS += win32
else ifeq ($(findstring MINGW32, $(OS)), MINGW)
	ASMFLAGS += win32
else ifeq ($(findstring MINGW64, $(OS)), MINGW64)
	ASMFLAGS += win64
else ifeq ($(OS), Linux)
	ASMFLAGS += elf64
endif

# === END TOOLS ===
# === TARGETS ===
TARGET := $(build_dir)/pbfs-cli

all: $(TARGET)

$(TARGET): $(C_SRCFILES) $(H_SRCFILES)
	@printf "$(CLR_YELLOW)-- Compiling Sources....$(CLR_RESET)\n"

	@mkdir -p $(build_dir)
	@$(CC) $(CFLAGS) -o $@ $(C_SRCFILES) $(CFLAGS_LIB)

	@printf "$(CLR_GREEN)-- Compiled to \"$@\"$(CLR_RESET)\n"

clean:
	@printf "$(CLR_RED)-- Cleaning...$(CLR_RESET)\n"
	@rm -rf $(build_dir)
	@printf "$(CLR_GREEN)-- Cleaned$(CLR_RESET)\n"

print-%:
	@echo $* = $($*)
