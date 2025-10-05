#include <pbfs_blt_stub.h>

void stage2_main(void) {
    PM_Cursor_t cur = {
        .x = 0,
        .y = 0,
        .bg = PM_COLOR_BLACK,
        .fg = PM_COLOR_WHITE
    };
    pm_clear_screen(&cur);
    pm_print(&cur, "PBFS Stage 2 is Loaded\n");
    pm_print(&cur, "Welcome to PBFS Testing Bootloader!\n");

    // the pbfs bootloder stub has no init, the kernel has to use the actual full pbfs
    pm_print(&cur, "Reading Kernel...SRY NOT NOW\n");
    for (;;) asm("hlt");
}
