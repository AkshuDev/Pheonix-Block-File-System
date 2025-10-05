// This file is specifically defined for real_mode and bootloaders
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0

#pragma pack(push, 1)
typedef struct {
    uint8_t size;
    uint8_t reserved;
    uint16_t sector_count;
    uint16_t offset;
    uint16_t segment;
    uint64_t lba;
} __attribute__((packed)) PBFS_DAP;
#pragma pack(pop)

extern int read_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern int write_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern uint16_t get_drive_params_real(uint16_t buffer_seg_offset, uint8_t drive);

#define READ_LBA read_lba_asm
#define WRITE_LBA write_lba_asm
#define PBFS_GET_DRIVE_PARAMETERS get_drive_params_real
