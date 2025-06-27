# === CONFIG ===
BaseDir := ..

build_dir := $(BaseDir)/build-cli
headers_dir := $(BaseDir)/headers

PEFI_dir := $(BaseDir)/PEFI
Pefi_headers_dir := $(PEFI_dir)/includes

C_SRCFILES = $(wildcard *.c)
ASM_SRCFILES = $(wildcard $(BaseDir)/tools/*.asm)

C_OBJS = $(patsubst %.c, $(build_dir)/%.o, $(C_SRCFILES))
ASM_OBJS = $(patsubst $(BaseDir)/tools/%.asm, $(build_dir)/%.o, $(ASM_SRCFILES))

OBJS := $(C_OBJS) $(ASM_OBJS)

# === END CONFIG ===
# === TOOLS ===
CC := gcc
CFLAGS := -I$(headers_dir) -I$(Pefi_headers_dir) -Wall -g -w
CFLAGS_LIB := -L$(PEFI_dir)/build -lpefi

ASM := nasm
ASMFLAGS := -f

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
TARGET := $(build_dir)/pbfs-cli.exe

all: $(TARGET)

$(TARGET): $(ASM_OBJS) $(C_SRCFILES)
	$(CC) $(CFLAGS) -o $@ $(ASM_OBJS) $(C_SRCFILES) $(CFLAGS_LIB)

# Assemble the base files
$(build_dir)/%.o: $(BaseDir)/tools/%.asm
	@mkdir -p $(build_dir)
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(build_dir)

print-%:
	@echo $* = $($*)
