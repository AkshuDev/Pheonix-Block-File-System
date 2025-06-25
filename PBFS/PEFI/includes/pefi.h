#pragma once

typedef uint64_t EFI_STATUS;
typedef void* EFI_HANDLE;

typedef unsigned short CHAR16;
typedef char CHAR8;

typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

typedef void VOID;

typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef UINT64 UINTN;

typedef long long INT64;
typedef int INT32;
typedef short INT16;
typedef signed char INT8;
typedef INT64 INTN;

typedef unsigned char BOOLEAN;

typedef UINTN EFI_TPL;

typedef uint64_t EFI_LBA;

typedef UINTN RETURN_STATUS;


#define EFIAPI

#define uefi_call_wrapper(func, va_num, ...) func(__VA_ARGS__)

#define MAX_BIT 0x8000000000000000ULL
#define MAX_2_BITS 0xC000000000000000ULL
#define MAX_ADDRESS 0xFFFFFFFFFFFFFFFFULL
#define MAX_ALLOC_ADDRESS 0xFFFFFFFFFFFFULL
#define MAX_INTN ((INTN)0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINTN ((UINTN)0xFFFFFFFFFFFFFFFFULL)
#define MIN_INTN (((INTN)-9223372036854775807LL) - 1)
#define CPU_STACK_ALIGNMENT 16
#define DEFAULT_PAGE_ALLOCATION_GRANULARITY (0x1000)
#define RUNTIME_PAGE_ALLOCATION_GRANULARITY (0x10000)

#define __USER_LABEL_PREFIX__

#define FUNCTION_ENTRY_POINT (FunctionPointer)(VOID *)(UINTN)(FunctionPointer)

#define IN
#define OUT
#define IN OUT
#define OPTIONAL
#define CONST const
#define FALSE 0

#define EFI_ERROR(Status) (((INTN)(Status)) < 0)
#define ENCODE_ERROR	(StatusCode)((RETURN_STATUS)(MAX_BIT | (StatusCode)))
#define RETURN_ACCESS_DENIED   ENCODE_ERROR (15)
#define EFI_ACCESS_DENIED   RETURN_ACCESS_DENIED

#ifdef _MSC_VER
  #define PEFI_PACKED_STRUCT(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))
#else
  #define PEFI_PACKED_STRUCT(decl) decl __attribute__((packed))
#endif


PEFI_PACKED_STRUCT(typedef struct {
    UINT16  Year;         // 2 bytes (e.g. 2025)
    UINT8   Month;        // 1 byte (1–12)
    UINT8   Day;          // 1 byte (1–31)
    UINT8   Hour;         // 1 byte (0–23)
    UINT8   Minute;       // 1 byte (0–59)
    UINT8   Second;       // 1 byte (0–59)
    UINT8   Pad1;         // 1 byte (alignment padding)
    UINT32  Nanosecond;   // 4 bytes (0–999,999,999)
    INT16   TimeZone;     // 2 bytes (UTC offset in minutes)
    UINT8   Daylight;     // 1 byte (bitmask)
    UINT8   Pad2;         // 1 byte (alignment)
} EFI_TIME;)

typedef struct {
    UINT32  Resolution;      // 1Hz = 1, 1000Hz = 1000, etc.
    UINT32  Accuracy;        // +/- n seconds
    BOOLEAN SetsToZero;      // TRUE if setting time resets other fields
} EFI_TIME_CAPABILITIES;

PEFI_PACKED_STRUCT(typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} EFI_GUID;)

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

PEFI_PACKED_STRUCT(typedef struct {
    EFI_GUID  CapsuleGuid;     // 16 bytes
    UINT32    HeaderSize;      // 4 bytes
    UINT32    Flags;           // 4 bytes
    UINT32    CapsuleImageSize;// 4 bytes
} EFI_CAPSULE_HEADER;)

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

PEFI_PACKED_STRUCT(typedef struct {
    UINT32 Type;          // EFI_MEMORY_TYPE
    UINT32 Pad;           // padding for alignment
    EFI_PHYSICAL_ADDRESS PhysicalStart;// 8 bytes
    EFI_VIRTUAL_ADDRESS VirtualStart; // 8 bytes
    UINT64 NumberOfPages; // 8 bytes
    UINT64 Attribute;     // 8 bytes (bitmask)
} EFI_MEMORY_DESCRIPTOR;)

typedef enum {
    TimerCancel,
    TimerPeriodic,
    TimerRelative
} EFI_TIMER_DELAY;

typedef enum {
    EFI_NATIVE_INTERFACE
} EFI_INTERFACE_TYPE;

typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

PEFI_PACKED_STRUCT(typedef struct {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2]; // Length = (UINT16) packed as [lo, hi]
} EFI_DEVICE_PATH_PROTOCOL;)

typedef struct {
    EFI_HANDLE AgentHandle;         // 8 bytes
    EFI_HANDLE ControllerHandle;    // 8 bytes
    UINT32 Attributes;              // 4 bytes
    UINT32 OpenCount;               // 4 bytes
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

PEFI_PACKED_STRUCT(typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;)

typedef struct {
    EFI_GUID VendorGuid; // 16 bytes
    VOID *VendorTable;   // 8 bytes
} EFI_CONFIGURATION_TABLE;


typedef VOID* EFI_EVENT;
typedef VOID (EFIAPI *EFI_EVENT_NOTIFY) (IN EFI_EVENT Event, IN VOID *Context);

typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT) (IN UINT32 Type, IN EFI_TPL NotifyTpl, IN EFI_EVENT_NOTIFY NotifyFunction OPTIONAL, IN VOID *NotifyContext OPTIONAL, OUT EFI_EVENT *Event);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(IN EFI_LOCATE_SEARCH_TYPE SearchType, IN EFI_GUID *Protocol OPTIONAL, IN VOID *SearchKey OPTIONAL, IN OUT UINTN *BufferSize, OUT EFI_HANDLE *Buffer);

// Thanks to dox.ipxe.org for helping me resolve structs and more.