#include "debug.h"
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/kmalloc.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/cpu.h>
#include <stdio.h>
#include <stdlib.h>

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

void pe_debug_init(void)
{
    usb_interface_t const *intf[] = { &usb_ff_bulk, NULL };
    usb_open(intf, GINT_CALL_NULL);

    dclear(C_BLACK);
    dtext_opt(DWIDTH/2, DHEIGHT/2 - 3, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_BOTTOM, "Waiting for USB connection...");
    dtext_opt(DWIDTH/2, DHEIGHT/2 + 3, C_WHITE, C_NONE, DTEXT_MIDDLE,
        DTEXT_TOP, "% fxlink -iqw");
    dupdate();

    while(!usb_is_open()) sleep();
}

void pe_debug_printf(char const *fmt, ...)
{
    char str_default[256];

    va_list args;
    va_start(args, fmt);
    char *str;
    vasprintf(&str, fmt, args);
    va_end(args);

    if(str == NULL) {
        va_start(args, fmt);
        str = str_default;
        vsnprintf(str, sizeof str_default, fmt, args);
        va_end(args);
    }

    if(usb_is_open())
        usb_fxlink_text(str, 0);
}

/* This function is used in MicroPython. */
void DEBUG_printf(char const *fmt, ...)
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
    if(usb_is_open())
        usb_fxlink_screenshot(true);
}

#endif /* PE_DEBUG */
