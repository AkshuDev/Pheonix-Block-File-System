// HI, This is a driver for UEFI Only
#include <pefilib.h>
#include <pefi_types.h>
#include <pefi.h>
#include <pefi_priv.h>

int INIT_EFI(EFI_SYSTEM_TABLE* SystemTable, EFI_HANDLE ImageHandle) {
	InitalizeLib(SystemTable, ImageHandle);
}

EFI_BLOCK_IO_PROTOCOL *GBlockIo = NULL;

EFI_STATUS FindFirstBlockDevice() {
	EFI_STATUS status;
	UINTN HandleCount = 0;
	EFI_HANDLE *handles = NULL;

	status = gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiBlockIoProtocolGuid,
		NULL,
		&HandleCount,
		&handles
	);

	if (EFI_ERROR(status) || HandleCount == 0) return -1; // Not found

	status = gBS->HandleProtocol(handles[0], &gEfiBlockIoProtocolGuid, (void**)&GBlockIo);
	gBS->FreePool(handles);

	return status;
}

EFI_STATUS Read_Block_LBA(UINT64 lba, void* buffer) {
	if (!GBlockIo) {
		EFI_STATUS status = FindFirstBlockDevice();
		if (EFI_ERROR(status)) return status;
	}

	EFI_BLOCK_IO_PROTOCOL *bio = GBlockIo;
	UINT32 mediaId = bio->Media->MediaId;
	UINTN blockSize = bio->Media->BlockSize;

	return bio->ReadBlocks(bio, mediaId, lba, blockSize, buffer);
}
}
