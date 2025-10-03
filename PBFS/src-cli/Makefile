# === CONFIG ===
BaseDir := ..

build_dir := $(BaseDir)/build-cli
obj_dir = $(build_dir)/objs
headers_dir := $(BaseDir)/headers

PEFI_dir := $(BaseDir)/PEFI
Pefi_headers_dir := $(PEFI_dir)/includes

C_SRCFILES = $(wildcard *.c)
H_SRCFILES = $(wildcard $(headers_dir)/*.h)
ASM_SRCFILES = $(wildcard $(BaseDir)/tools/*.asm)

C_OBJS = $(patsubst %.c, $(build_dir)/%.o, $(C_SRCFILES))
ASM_OBJS = $(patsubst $(BaseDir)/tools/%.asm, $(obj_dir)/%.o, $(ASM_SRCFILES))

OBJS := $(C_OBJS) $(ASM_OBJS)

# === END CONFIG ===
# === TOOLS ===
CC := gcc
CFLAGS := -I$(headers_dir) -I$(Pefi_headers_dir) -Wall -g -w
CFLAGS_LIB := -L$(PEFI_dir)/build -lpefi -Wl,-rpath,$(realpath $(PEFI_dir)/build)

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
TARGET := $(build_dir)/pbfs-cli

all: $(TARGET)

$(TARGET): $(ASM_OBJS) $(C_SRCFILES) $(H_SRCFILES)
	$(CC) $(CFLAGS) -o $@ $(ASM_OBJS) $(C_SRCFILES) $(CFLAGS_LIB)

# Assemble the base files
$(obj_dir)/%.o: $(BaseDir)/tools/%.asm
	@mkdir -p $(build_dir)
	@mkdir -p $(obj_dir)
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(build_dir)

print-%:
	@echo $* = $($*)
