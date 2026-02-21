# === CONFIG ===
BaseDir := ..

build_dir := $(BaseDir)/build-cli
obj_dir = $(build_dir)/objs
headers_dir := $(BaseDir)/headers

C_SRCFILES = $(wildcard *.c) $(BaseDir)/tools/utils.c
H_SRCFILES = $(wildcard $(headers_dir)/*.h)

C_OBJS = $(patsubst %.c, $(build_dir)/%.o, $(C_SRCFILES))
OBJS := $(C_OBJS) $(ASM_OBJS)

# === END CONFIG ===
# === TOOLS ===
CC := gcc
CFLAGS := -I$(headers_dir) -Wall -g -w
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
	$(CC) $(CFLAGS) -o $@ $(C_SRCFILES) $(CFLAGS_LIB)

clean:
	rm -rf $(build_dir)

print-%:
	@echo $* = $($*)
