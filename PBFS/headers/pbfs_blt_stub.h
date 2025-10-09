// This header file is different from C file because this also has special funcs for printing and such in protected mode!

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
} __attribute__((packed)) PBFS_DAP; // 64-bit cause BIOS can do it
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint64_t lba;
    uint16_t count;
} __attribute__((packed)) PBFS_DP; // PBFS DP (Disk Packet) differs from DAP as the DP supports protected/long mode

extern int read_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern int write_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern uint16_t get_drive_params_real(uint16_t buffer_seg_offset, uint8_t drive);

#define read_lba_real read_lba_asm
#define write_lba_real write_lba_asm
#define get_drive_params_real get_drive_params_real

// Define I/O port addresses for the primary ATA bus.
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

// I/O ports relative to the base address
#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_SECTOR_COUNT 0x02
#define ATA_REG_LBA_LOW 0x03
#define ATA_REG_LBA_MID 0x04
#define ATA_REG_LBA_HIGH 0x05
#define ATA_REG_DRIVE_HEAD 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07

// Status Register Bits
#define ATA_SR_BSY 0x80 // Busy
#define ATA_SR_DRDY 0x40 // Drive Ready
#define ATA_SR_DF 0x20 // Device Fault
#define ATA_SR_DRQ 0x08 // Data Request Ready
#define ATA_SR_ERR 0x01 // Error

// ATA Commands
#define ATA_CMD_READ_PIO_EXT 0x24 // LBA48 Read
#define ATA_CMD_WRITE_PIO_EXT 0x34 // LBA48 Write
#define ATA_CMD_FLUSH_EXT 0xEA // LBA48 Flush

// ATA Ports
#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_ERROR 0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LOW 0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HIGH 0x1F5
#define ATA_PRIMARY_DRIVE_HEAD 0x1F6
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_PRIMARY_STATUS 0x1F7 // Status register is at the same address as Command
#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376


// Developer friendly zone
#define VIDEO_MEMORY 0xB8000
#define VIDEO_WIDTH 80
#define VIDEO_HEIGHT 25

typedef enum {
    PM_COLOR_BLACK = 0x0,
    PM_COLOR_BLUE = 0x1,
    PM_COLOR_GREEN = 0x2,
    PM_COLOR_CYAN = 0x3,
    PM_COLOR_RED = 0x4,
    PM_COLOR_MAGENTA = 0x5,
    PM_COLOR_BROWN = 0x6,
    RM_COLOR_LIGHT_GRAY = 0x7,
    PM_COLOR_DARK_GRAY = 0x8,
    PM_COLOR_LIGHT_BLUE = 0x9,
    PM_COLOR_LIGHT_GREEN = 0xA,
    PM_COLOR_LIGHT_CYAN = 0xB,
    PM_COLOR_LIGHT_RED = 0xC,
    PM_COLOR_LIGHT_MAGENTA = 0xD,
    PM_COLOR_YELLOW = 0xE,
    PM_COLOR_WHITE = 0xF
} PM_Colors_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    PM_Colors_t fg;
    PM_Colors_t bg;
} PM_Cursor_t;

// Output a single byte to a port
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ (
        "outb %0, %1"
        :
        : "a"(value),"Nd"(port)
    );
}

// Take input of 1 byte value from port
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ __volatile__ (
        "inb %1, %0"
        : "=a"(result)
        : "dN"(port)
    );
    return result;
}

// Take input of 2 byte value from port
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__(
        "inw %1, %0"
        : "=a"(ret) 
        : "dN"(port)
    );
    return ret;
}

// Output a 2 byte value to port
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__(
        "outw %0, %1" 
        : 
        : "a"(val),"dN"(port)
    );
}

// Take input from port
static inline void insw(uint16_t port, void* buffer, uint32_t count) {
    __asm__ __volatile__(
        "rep insw" 
        : "+D"(buffer), "+c"(count) 
        : "d"(port) 
        : "memory"
    );
}

// Output buffer in port
static inline void outsw(uint16_t port, const void* buffer, uint32_t count) {
    __asm__ __volatile__(
        "rep outsw" 
        : "+S"(buffer), "+c"(count) 
        : "d"(port)
    );
}

// ATA
typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
} ata_bus_t;

static const ata_bus_t ata_buses[2] = {
    { ATA_PRIMARY_IO, ATA_PRIMARY_CTRL },
    { ATA_SECONDARY_IO, ATA_SECONDARY_CTRL }
};

// Wait for ATA (PRIVATE DO NOT USE UNLESS YOU KNOW WHAT YOU ARE DOING)
static inline void _ata_wait_ready(uint16_t io_base) {
    uint8_t status;
    int timeout = 100000; // ~100ms depending on CPU speed

    while (timeout--) {
        status = inb(io_base + 7);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY))
            return; // Ready!
    }
}

// Read using ATA (slow for modern disks but fast enough for bootloader/protected mode)
static inline int pm_read_sectors(PBFS_DP* dp, void* buffer, uint8_t drive) {
    uint8_t bus = ((drive - 0x80) >> 1) & 1;
    uint8_t device = (drive & 1); // 0=master,1=slave
    uint16_t io = ata_buses[bus].io_base;
    uint16_t ctrl = ata_buses[bus].ctrl_base;

    _ata_wait_ready(io);

    // Send the LBA48 and sector count high byte first
    outb(ctrl, 0x00); // Clear high order bytes
    outb(io + 2, (dp->count >> 8) & 0xFF);
    outb(io + 3, (dp->lba >> 24) & 0xFF);
    outb(io + 4, (dp->lba >> 32) & 0xFF);
    outb(io + 5, (dp->lba >> 40) & 0xFF);

    // Send the LBA48 low bytes and sector count low byte.
    outb(io + 6, 0x40 | (device << 4)); // Choose drive

    outb(io + 2, dp->count & 0xFF);
    outb(io + 3, dp->lba & 0xFF);
    outb(io + 4, (dp->lba >> 8) & 0xFF);
    outb(io + 5, (dp->lba >> 16) & 0xFF);

    // Issue the LBA48 Read command.
    outb(io + 7, ATA_CMD_READ_PIO_EXT);

    // Read each sector.
    for (int i = 0; i < dp->count; i++) {
        _ata_wait_ready(io);
        uint8_t status = inb(io + 7);

        // Check for errors.
        if (status & ATA_SR_ERR) { // Error
            return -1;
        }

        // Wait for Data Request Ready.
        while (!(inb(io + 7) & ATA_SR_DRQ));

        // Read 256 words (512 bytes) into the buffer.
        insw(io, (uint8_t*)buffer + (uint32_t)i * 512, 256);
    }

    return 0; // Success
}

// Write using ATA (slow for modern disks but fast enough for bootloader/protected mode)
static inline int pm_write_sectors(PBFS_DP* dp, const void* buffer, uint8_t drive) {
    uint8_t bus = ((drive - 0x80) >> 1) & 1;
    uint8_t device = (drive & 1);
    uint16_t io = ata_buses[bus].io_base;
    uint16_t ctrl = ata_buses[bus].ctrl_base;

    _ata_wait_ready(io);

    // Same as read
    outb(ctrl, 0x00);
    outb(io + 2, (dp->count >> 8) & 0xFF);
    outb(io + 3, (dp->lba >> 24) & 0xFF);
    outb(io + 4, (dp->lba >> 32) & 0xFF);
    outb(io + 5, (dp->lba >> 40) & 0xFF);

    outb(io + 6, 0x40 | (device << 4));
    outb(io + 2, dp->count & 0xFF);
    outb(io + 3, dp->lba & 0xFF);
    outb(io + 4, (dp->lba >> 8) & 0xFF);
    outb(io + 5, (dp->lba >> 16) & 0xFF);

    // Issue the LBA48 Write command.
    outb(io + 7, ATA_CMD_WRITE_PIO_EXT);

    // Write each sector.
    for (int i = 0; i < dp->count; i++) {
        _ata_wait_ready(io);
        uint8_t status = inb(io + 7);

        if (status & ATA_SR_ERR) {
            return -1;
        }

        while (!(inb(io + 7) & ATA_SR_DRQ));

        // Write 256 words (512 bytes) from the buffer.
        outsw(io, (uint8_t*)buffer + (uint32_t)i * 512, 256);
    }

    // Flush the cache to ensure data is written to disk.
    outb(io + 7, ATA_CMD_FLUSH_EXT);
    _ata_wait_ready(io);

    return 0; // Success
}

// Clear screen (VIDEO MEMORY)
static inline void pm_clear_screen(PM_Cursor_t* cursor) {
    uint16_t* vid = (uint16_t*)VIDEO_MEMORY;
    uint16_t attr = (cursor->bg << 4) | cursor->fg;
    for (uint16_t i = 0; i < VIDEO_WIDTH*VIDEO_HEIGHT; i++) vid[i] = attr << 8;
    cursor->x = 0;
    cursor->y = 0;
}

// Sets the cursor struct (changes cursor location as well)
static inline void pm_set_cursor(PM_Cursor_t* cursor, uint16_t x, uint16_t y) {
    uint16_t pos = y * VIDEO_WIDTH + x;
    // high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);

    // low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);

    cursor->x = x;
    cursor->y = y;
}

// Disable the cursor
static inline void pm_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // disable
}

// Seria out a single character
static inline void serial_put_char(char c) {
    // Wait for the serial port to be ready
    while ((inb(0x3F8 + 5) & 0x20) == 0); // Line Status Register, bit 5 = THR empty
    outb(0x3F8, c);
}

// Print a single character
static inline void pm_put_char(PM_Cursor_t* cursor, char c) {
    uint16_t* vid = (uint16_t*)VIDEO_MEMORY;
    uint16_t attr = (cursor->bg << 4) | cursor->fg;
    if (c == '\n') {
        cursor->x = 0;
        uint16_t y = cursor->y + 1;
        pm_set_cursor(cursor, cursor->x, y);
        serial_put_char(c);
        return;
    }
    vid[cursor->y * VIDEO_WIDTH + cursor->x] = ((uint16_t)attr << 8) | c;
    uint16_t x = cursor->x + 1;
    pm_set_cursor(cursor, x, cursor->y);
    serial_put_char(c);
}

// Print string
static inline void pm_print(PM_Cursor_t* cursor, const char* str) {
    while (*str) pm_put_char(cursor, *str++);
}

// Convert integer to string
static inline void pm_itoa(int val, char* buf) {
    int i = 0;
    if (val == 0) { buf[i++] = '0'; buf[i] = 0; return; }
    int neg = 0;
    if (val < 0) { neg = 1; val = -val; }
    char tmp[12];
    int j = 0;
    while(val) { tmp[j++] = '0' + (val % 10); val /= 10; }
    if (neg) tmp[j++] = '-';
    while(j--) buf[i++] = tmp[j];
    buf[i] = 0;
}

static inline void pm_utoa32(uint32_t val, char* buf) {
    char tmp[11];
    int i = 0;
    if (val == 0) { buf[i++] = '0'; buf[i] = 0; return; }
    while (val) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

static inline void pm_utoa64(uint64_t val, char* buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }

    char tmp[21];
    int i = 0;

    uint32_t parts[3]; // We’ll split 64-bit into two 32-bit parts + remainder
    parts[0] = (uint32_t)(val >> 32); // high
    parts[1] = (uint32_t)(val & 0xFFFFFFFF); // low

    while (parts[0] != 0 || parts[1] != 0) {
        // Manual division by 10 using only 32-bit math
        uint32_t remainder = 0;
        for (int j = 0; j < 2; j++) {
            uint64_t cur = ((uint64_t)remainder << 32) | parts[j];
            // simulate 64-bit /10 using 32-bit arithmetic
            uint32_t q = 0;
            uint32_t r = 0;

            // Binary long division
            for (int k = 31; k >= 0; k--) {
                r <<= 1;
                r |= (cur >> k) & 1;
                if (r >= 10) { r -= 10; q |= (1U << k); }
            }

            parts[j] = q;
            remainder = r;
        }
        tmp[i++] = '0' + remainder;
    }

    // Reverse digits
    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

static inline void pm_itoa_hex64(uint64_t val, char* buf) {
    const char* digits = "0123456789ABCDEF";
    char tmp[17];
    int i = 0;

    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }

    while (val) {
        tmp[i++] = digits[val & 0xF];
        val >>= 4;
    }

    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

// Print with format capabilities
static inline void pm_printf(PM_Cursor_t* cursor, const char* fmt, int first_arg_addr, ...) {
    char* argp = (char*)&first_arg_addr;
    char ch;

    while ((ch = *fmt++)) {
        if (ch != '%') {
            pm_put_char(cursor, ch);
            continue;
        }

        char next = *fmt++;
        if (!next) break;

        switch (next) {
            case 's': {
                const char* str = *((const char**)argp);
                argp += 4;
                pm_print(cursor, str ? str : "(null)");
                break;
            }
            case 'c': {
                char c = *((char*)argp);
                argp += 4;
                pm_put_char(cursor, c);
                break;
            }
            case 'd': {  // signed int
                int val = *((int*)argp);
                argp += 4;
                char buf[16];
                pm_itoa(val, buf);
                pm_print(cursor, buf);
                break;
            }
            case 'u': {  // unsigned 32-bit
                uint32_t val = *((uint32_t*)argp);
                argp += 4;
                char buf[16];
                pm_utoa32(val, buf);
                pm_print(cursor, buf);
                break;
            }
            case 'U': {  // unsigned 64-bit
                uint64_t val = *((uint64_t*)argp);
                argp += 8;
                char buf[32];
                pm_utoa64(val, buf);
                pm_print(cursor, buf);
                break;
            }
            case 'x': {  // hex (32-bit)
                uint32_t val = *((uint32_t*)argp);
                argp += 4;
                char buf[12];
                pm_itoa_hex64(val, buf);
                pm_print(cursor, buf);
                break;
            }
            case 'X': {  // hex (64-bit)
                uint64_t val = *((uint64_t*)argp);
                argp += 8;
                char buf[24];
                pm_itoa_hex64(val, buf);
                pm_print(cursor, buf);
                break;
            }
            case 'p': {  // pointer
                uint32_t ptr = *((uint32_t*)argp);
                argp += 4;
                char buf[12];
                pm_print(cursor, "0x");
                pm_itoa_hex64(ptr, buf);
                pm_print(cursor, buf);
                break;
            }
            case '%': {
                pm_put_char(cursor, '%');
                break;
            }
            default: {
                // Unknown specifier — print literally
                pm_put_char(cursor, '%');
                pm_put_char(cursor, next);
                break;
            }
        }
    }
}
// Scan Codes (SIMPLE FOR BOOTLOADER/PROTECTED MODE)
static const char scan_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// Reads via PS2
static inline uint8_t pm_ps2_read_scan(void) {
    while (!(inb(0x64) & 1)); // wait for data
    return inb(0x60);
}

// Read a line
static inline void pm_read_line(char* buf, int max_len, PM_Cursor_t* cursor) {
    int idx = 0;
    for (;;) {
        uint8_t sc = pm_ps2_read_scan(); // read a scan code
        char c = 0;

        // Ignore key releases (high bit set)
        if (sc & 0x80) continue;

        c = scan_to_ascii[sc];
        if (!c) continue;

        if (c == '\n') { // Enter
            buf[idx] = 0;
            pm_put_char(cursor, '\n');
            break;
        } else if (c == '\b') { // Backspace
            if (idx > 0) {
                idx--;
                // Move cursor back visually
                if (cursor->x == 0 && cursor->y > 0) {
                    cursor->y--;
                    cursor->x = VIDEO_WIDTH - 1;
                } else if (cursor->x > 0) {
                    cursor->x--;
                }
                pm_set_cursor(cursor, cursor->x, cursor->y);
                pm_put_char(cursor, ' '); // erase
                cursor->x--;
                pm_set_cursor(cursor, cursor->x, cursor->y);
            }
        } else {
            if (idx < max_len - 1) {
                buf[idx++] = c;
                pm_put_char(cursor, c); // echo typed char
            }
        }
    }
}

// Compare two null-terminated strings
static inline int str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b; // both must be 0
}

// Compare first n chars of two strings
static inline int str_n_eq(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
        if (a[i] == 0 || b[i] == 0) return 1; // both must be valid
    }
    return 1;
}

// Find length of string
static inline int str_len(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

static inline int str_prefix(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}
