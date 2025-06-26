# === CONFIG ===
BaseDir := ..

build_dir := $(BaseDir)/build-cli
headers_dir := $(BaseDir)/headers

PEFI_dir := $(BaseDir)/PEFI
Pefi_headers_dir := $(PEFI_dir)/includes

SRCFILES = $(wildcard *.c)
OBJS = $(patsubst %.c, $(build_dir)/%.o, $(SRCFILES))

# === END CONFIG ===
# === TOOLS ===
CC := gcc
CFLAGS := -I$(headers_dir) -I$(Pefi_headers_dir) -Wall -g -L$(PEFI_dir)/build -lpefi
CFLAGS_COMPILE := -I$(headers_dir) -I$(Pefi_headers_dir) -c -g

# === END TOOLS ===
# === TARGETS ===
TARGET := $(build_dir)/pbfs-cli.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile .c -> .o
$(build_dir)/%.o: %.c
	@mkdir -p $(build_dir)
	$(CC) $(CFLAGS_COMPILE) $< -o $@

clean:
	rm -rf $(build_dir)

print-%:
	@echo $* = $($*)
