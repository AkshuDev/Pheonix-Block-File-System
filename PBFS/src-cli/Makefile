# === CONFIG ===
Cdir := $(shell pwd)/..
CLIdir := $(Cdir)/src-cli

build_dir := $(Cdir)/build-cli

headers_dir := $(Cdir)/headers

# === END CONFIG ===
# === TOOLS ===
CC := gcc
CFLAGS := -I$(headers_dir) -Wall -g

# === END TOOLS ===
# === TARGETS ===
TARGET := $(build_dir)/pbfs-cli.exe

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(CLIdir)/pbfs-cli.c
	mkdir -p $(build_dir)
	$(CC) $(CFLAGS) $(CLIdir)/pbfs-cli.c -o $(TARGET)

clean:
	rm -rf $(build_dir)