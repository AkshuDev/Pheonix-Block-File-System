# Variables
Cdir := $(shell pwd)
pbfs_main_fs := $(Cdir)/src-fs/pbfs.c
pbfs_main_fs_out := $(Cdir)/src-fs/pbfs.o

HeadersDir := $(Cdir)/headers
PefiDir := $(Cdir)

tool_disk_io_o := $(Cdir)/build-fs/disk_io.o
tool_disk_params_o := $(Cdir)/build-fs/disk_params.o

CC = gcc
CFLAGS = -ffreestanding -nostartfiles -fno-pic $(pbfs_main_fs) $(tool_disk_io_o) $(tool_disk_params_o) -o $(pbfs_main_fs_out) -I$(HeadersDir) -I$(PefiDir) -L$(PefiLibDir) -lpefi

