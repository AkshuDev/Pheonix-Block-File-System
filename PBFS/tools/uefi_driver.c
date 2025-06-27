// HI, This is a driver for UEFI Only
// NOTE: This driver is a real driver but is just a backup as PEFI already contains all this in its library file.
#include <pefilib.h>
#include <pefi_types.h>
#include <pefi.h>
#include <pefi_priv.h>

int INIT_EFI(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) {
	InitalizeLib(SystemTable, ImageHandle);
}

EFI_BLOCK_IO_PROTOCOL *GBlockIo = NULL;

int EFI_LOCATE_BLOCK_IO(EFI_SYSTEM_TABLE* SystemTable) {
	EFI_STATUS status;
	EFI_HANDLE *handles;
	UINTN handle_count;

	status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &handle_count, &handles);

	if (EFI_ERROR(status)) return -1; // ERROR

	for (UINTN i = 0; i < handle_count; i++) {
		status = gBS->HandleProtocol(handles[i], &gEfiBlockIoProtocolGuid, (void**)&GBlockIo);

		if (!EFI_ERROR(status)) break;
	}

	gBS->FreePool(handles);

	return 0; // SUCCESS
}

EFI_STATUS EFI_READ_LBA(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer) {
	if (!GBlockIo || !buffer) return -1; // ERROR

	EFI_STATUS status = GBlockIo->ReadBlocks(GBlockIo, GBlockIo->Media->MediaId, lba, count * GBlockIo->Media->BlockSize, buffer);

	if (EFI_ERROR(status)) return -2; // ERROR 2

	return status;
}

EFI_STATUS EFI_WRITE_LBA(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer) {
	if (!GBlockIo || !buffer) return -1; // ERROR

	EFI_STATUS status = GBlockIo->WriteBlocks(GBlockIo, GBlockIo->Media->MediaId, lba, count * GBlockIo->Media->BlockSize, buffer);

	if (EFI_ERROR(status)) return -2; // ERROR 2

	return status;
}
