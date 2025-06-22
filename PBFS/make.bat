@echo off
setlocal ENABLEEXTENSIONS

REM === CONFIG ===
set Cdir=%~dp0..
set CLIdir=%Cdir%\src-cli
set build_cli=%Cdir%\build-cli
set headers_dir=%Cdir%\headers

set BuildDir=%Cdir%\build-fs
set pbfs_main_fs=%Cdir%\src-fs\pbfs.c
set pbfs_main_fs_out=%BuildDir%\pbfs.o

set HeadersDir=%Cdir%\headers
set PefiDir=%Cdir%\PEFI
set PefiHeaderDir=%PefiDir%\includes
set PefiLibDir=%PefiDir%\build

set tool_disk_io_o=%BuildDir%\disk_io.o
set tool_disk_params_o=%BuildDir%\disk_params.o

set tool_disk_io=%Cdir%\tools\disk_io.asm
set tool_disk_params=%Cdir%\tools\disk_params.asm

set CLI_TARGET=%build_cli%\pbfs-cli.exe
set PBFS_TARGET=%BuildDir%\pbfs.exe

REM === TOOLS ===
set CC=gcc
set LD=ld
set ASM=nasm

REM === FLAGS ===
set CFLAGS=-I%headers_dir% -Wall -g
set PBFS_CFLAGS=-ffreestanding -nostartfiles -fno-pic -nostdlib -I%HeadersDir% -I%PefiHeaderDir% -L%PefiLibDir% -lpefi
set LDFLAGS=-nostdlib
set ASMFLAGS=-f win64

REM === CREATE DIRECTORIES ===
if not exist "%build_cli%" mkdir "%build_cli%"
if not exist "%BuildDir%" mkdir "%BuildDir%"

echo === Building pbfs-cli ===
%CC% %CFLAGS% "%CLIdir%\pbfs-cli.c" -o "%CLI_TARGET%"
if errorlevel 1 goto :error

echo === Assembling disk_io.asm ===
%ASM% %ASMFLAGS% "%tool_disk_io%" -o "%tool_disk_io_o%"
if errorlevel 1 goto :error

echo === Assembling disk_params.asm ===
%ASM% %ASMFLAGS% "%tool_disk_params%" -o "%tool_disk_params_o%"
if errorlevel 1 goto :error

echo === Compiling PBFS main C ===
%CC% %PBFS_CFLAGS% "%pbfs_main_fs%" -o "%pbfs_main_fs_out%"
if errorlevel 1 goto :error

echo === Linking PBFS executable ===
%LD% %LDFLAGS% "%tool_disk_io_o%" "%tool_disk_params_o%" -o "%PBFS_TARGET%"
if errorlevel 1 goto :error

echo === Build Complete ===
goto :eof

:error
echo.
echo === BUILD FAILED ===
exit /b 1
