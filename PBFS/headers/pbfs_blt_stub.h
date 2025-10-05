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
} __attribute__((packed)) PBFS_DAP;
#pragma pack(pop)

extern int read_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern int write_lba_asm(PBFS_DAP* dap, uint8_t drive);
extern uint16_t get_drive_params_real(uint16_t buffer_seg_offset, uint8_t drive);

#define read_lba read_lba_asm
#define write_lba write_lba_asm
#define get_drive_params get_drive_params_real

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

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__ (
        "outb %0, %1"
        :
        : "a"(value),"Nd"(port)
    );
}

static inline void pm_clear_screen(PM_Cursor_t* cursor) {
    uint16_t* vid = (uint16_t*)VIDEO_MEMORY;
    uint16_t attr = (cursor->bg << 4) | cursor->fg;
    for (uint16_t i = 0; i < VIDEO_WIDTH*VIDEO_HEIGHT; i++) vid[i] = attr << 8;
    cursor->x = 0;
    cursor->y = 0;
}

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

static inline void pm_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20); // disable
}

static inline void pm_put_char(PM_Cursor_t* cursor, char c) {
    uint16_t* vid = (uint16_t*)VIDEO_MEMORY;
    uint16_t attr = (cursor->bg << 4) | cursor->fg;
    if (c == '\n') {
        cursor->x = 0;
        uint16_t y = cursor->y + 1;
        pm_set_cursor(cursor, cursor->x, y);
        return;
    }
    vid[cursor->y * VIDEO_WIDTH + cursor->x] = ((uint16_t)attr << 8) | c;
    uint16_t x = cursor->x + 1;
    pm_set_cursor(cursor, x, cursor->y);
}

static inline void pm_print(PM_Cursor_t* cursor, const char* str) {
    while (*str) pm_put_char(cursor, *str++);
}

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

static inline void pm_printf(PM_Cursor_t* cursor, const char* fmt, int first_arg_addr) {
    char* argp = (char*)&first_arg_addr;
    char ch;
    while ((ch = *fmt++)) {
        if (ch == '%') {
            char next = *fmt++;
            if (next == 's') {
                const char *str = *((const char**)argp);
                argp += 4; // move pointer by 32-bit width
                pm_print(cursor, str);
            } else if (next == 'd') {
                int val = *((int*)argp);
                argp += 4; // move to next arg
                char buf[12];
                pm_itoa(val, buf);
                pm_print(cursor, buf);
            } else {
                pm_put_char(cursor, ch);
            }
        }
    }
}
