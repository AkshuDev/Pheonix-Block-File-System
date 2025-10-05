#include <pbfs.h>
#include <pbfs_blt_stub.h>

void entry(void) {
    PM_Cursor_t cur = {
        .x = 0,
        .y = 0,
        .bg = PM_COLOR_BLACK,
        .fg = PM_COLOR_WHITE
    };
    pm_print(&cur, "Kernel has been loaded!\n");
}
