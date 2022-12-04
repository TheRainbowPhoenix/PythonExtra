#include "debug.h"
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/kmalloc.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include "py/mpstate.h"

void pe_debug_panic(char const *msg)
{
    int dy = dfont_default()->line_height + 2;

    dclear(C_BLACK);
    dtext(1, 1, C_WHITE, "PythonExtra panic!");
    dtext(1, 1+dy, C_WHITE, msg);
    dupdate();

    getkey();
    exit(1);
}

#if PE_DEBUG

#if 0 // Timeout fxlink not supported yet
static bool timeout_popup(void)
{
    dclear(C_BLACK);
    dtext_opt(DWIDTH/2, DHEIGHT/2 - 21, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_MIDDLE, "An fxlink message timed out!");
    dtext_opt(DWIDTH/2, DHEIGHT/2 - 7, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_MIDDLE, "Start fxlink and press [EXE]:");
    dtext_opt(DWIDTH/2, DHEIGHT/2 + 7, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_MIDDLE, "% fxlink -iqw");
    dtext_opt(DWIDTH/2, DHEIGHT/2 + 21, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_MIDDLE, "or press [EXIT] to drop the message");
    dupdate();

    while(1) {
        int key = getkey().key;
        if(key == KEY_EXE)
            return false;
        if(key == KEY_EXIT)
            return true;
    }
}
#endif

void pe_debug_init(void)
{
    usb_interface_t const *intf[] = { &usb_ff_bulk, NULL };
    usb_open(intf, GINT_CALL_NULL);
    usb_open_wait();
}

int pe_debug_printf(char const *fmt, ...)
{
    char str[512];
    va_list args;

    va_start(args, fmt);
    int rc = vsnprintf(str, sizeof str, fmt, args);
    va_end(args);

    usb_open_wait();
    usb_fxlink_text(str, 0);
    return rc;
}

/* This function is used in MicroPython. */
int DEBUG_printf(char const *fmt, ...)
__attribute__((alias("pe_debug_printf")));

void pe_debug_kmalloc(void)
{
    kmalloc_gint_stats_t *s;

    s = kmalloc_get_gint_stats(kmalloc_get_arena("_uram"));
    pe_debug_printf("[_uram] used=%d free=%d\n",
        s->used_memory, s->free_memory);

    s = kmalloc_get_gint_stats(kmalloc_get_arena("_ostk"));
    pe_debug_printf("[_ostk] used=%d free=%d\n",
        s->used_memory, s->free_memory);
}

void pe_debug_screenshot(void)
{
    usb_open_wait();
    usb_fxlink_screenshot(true);
}

#endif /* PE_DEBUG */
