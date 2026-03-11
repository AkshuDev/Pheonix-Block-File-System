#include <pefi_types.h>
#include <pefi_disk.h>

LIB EFI_BLOCK_IO_PROTOCOL* pefi_find_block_io(EFI_SYSTEM_TABLE* SystemTable) {
	EFI_STATUS status;
	EFI_HANDLE *handles;
	UINTN handle_count;
    EFI_BLOCK_IO_PROTOCOL* blockio = NULL;

	status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &handle_count, &handles);

	if (EFI_ERROR(status)) return NULL; // ERROR

	for (UINTN i = 0; i < handle_count; i++) {
		status = gBS->HandleProtocol(handles[i], &gEfiBlockIoProtocolGuid, (void**)&blockio);

		if (!EFI_ERROR(status)) break;
	}

	gBS->FreePool(handles);

	return 0; // SUCCESS
}

LIB EFI_STATUS pefi_read_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo) {
	if (!GBlockIo || !buffer) return -1; // ERROR

	EFI_STATUS status = GBlockIo->ReadBlocks(GBlockIo, GBlockIo->Media->MediaId, lba, count * GBlockIo->Media->BlockSize, buffer);

	if (EFI_ERROR(status)) return -2; // ERROR 2

	return status;
}

LIB EFI_STATUS pefi_write_lba(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle, EFI_LBA lba, UINTN count, void *buffer, EFI_BLOCK_IO_PROTOCOL* GBlockIo) {
	if (!GBlockIo || !buffer) return -1; // ERROR

	EFI_STATUS status = GBlockIo->WriteBlocks(GBlockIo, GBlockIo->Media->MediaId, lba, count * GBlockIo->Media->BlockSize, buffer);

	if (EFI_ERROR(status)) return -2; // ERROR 2

	return status;
}
