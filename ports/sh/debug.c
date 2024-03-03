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

struct pe_debug_meminfo pe_debug_startup_meminfo[PE_DEBUG_STARTUP_N];

void pe_debug_get_meminfo(struct pe_debug_meminfo *info)
{
    kmalloc_gint_stats_t *s;

#ifdef FX9860G
    s = kmalloc_get_gint_stats(kmalloc_get_arena("_uram"));
    info->_uram_used = s->used_memory;
    info->_uram_free = s->free_memory;

    s = kmalloc_get_gint_stats(kmalloc_get_arena("pram0"));
    info->pram0_used = s->used_memory;
    info->pram0_free = s->free_memory;
#endif

#ifdef FXCG50
    s = kmalloc_get_gint_stats(kmalloc_get_arena("_uram"));
    info->_uram_used = s->used_memory;
    info->_uram_free = s->free_memory;

    s = kmalloc_get_gint_stats(kmalloc_get_arena("_ostk"));
    info->_ostk_used = s->used_memory;
    info->_ostk_free = s->free_memory;
#endif
}

void pe_debug_browse_meminfo(void)
{
    dclear(C_WHITE);
    struct pe_debug_meminfo infonow;
    pe_debug_get_meminfo(&infonow);

#ifdef FX9860G
    static char const * const names[] = {
        "main", "cons.", "upy", "prom.", "ui",
        "now", /* extra element compared to original enum */
    };
    dprint(1, 0, C_BLACK, "Memory info");

    extern font_t font_3x5, font_4x6;
    font_t const *old_font = dfont(&font_4x6);
    dtext(33, 9, C_BLACK, "_uram");
    dtext(75, 9, C_BLACK, "pram0");
    dline(2, 16, DWIDTH-3, 16, C_BLACK);

    for(int i = 0; i < PE_DEBUG_STARTUP_N + 1; i++) {
        int y = 7 * i + 18;
        struct pe_debug_meminfo *info = &pe_debug_startup_meminfo[i];
        if(i >= PE_DEBUG_STARTUP_N) {
            dline(2, y, DWIDTH-3, y, C_BLACK);
            y += 2;
            info = &infonow;
        }

        dtext(2, y, C_BLACK, names[i]);
        dfont(&font_3x5);
        dprint(33, y+1, C_BLACK,
            "%d,%d", info->_uram_used, info->_uram_free);
        dprint(75, y+1, C_BLACK,
            "%d,%d", info->pram0_used, info->pram0_free);
        dfont(&font_4x6);
    }

    dfont(old_font);
#endif

#ifdef FXCG50
    static char const * const names[] = {
        "main", "console", "upy", "prompt", "ui",
        "now", /* extra element compared to original enum */
    };

    dtext(1, 1, C_BLACK, "Memory info");
    dtext(83, 24, C_BLACK, "_uram");
    dtext(170, 24, C_BLACK, "_ostk");
    dline(10, 36, DWIDTH-11, 36, C_BLACK);

    for(int i = 0; i < PE_DEBUG_STARTUP_N + 1; i++) {
        int y = 15 * i + 39;
        struct pe_debug_meminfo *info = &pe_debug_startup_meminfo[i];
        if(i >= PE_DEBUG_STARTUP_N) {
            dline(10, y, DWIDTH-11, y, C_BLACK);
            y += 4;
            info = &infonow;
        }

        dtext(10, y, C_BLACK, names[i]);
        dprint(83, y+1, C_BLACK,
            "%d,%d", info->_uram_used, info->_uram_free);
        dprint(170, y+1, C_BLACK,
            "%d,%d", info->_ostk_used, info->_ostk_free);
    }
#endif

    dupdate();
    while((getkey().key) != KEY_EXIT);
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

static bool videocapture = false;

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

static void print_strn(void *env, const char *str, size_t len) {
    (void)env;
    usb_open_wait();
    usb_fxlink_text(str, len);
}

mp_print_t const mp_debug_print = { NULL, print_strn };

void pe_debug_kmalloc(char const *prefix)
{
    kmalloc_gint_stats_t *s1, *s2;
    s1 = kmalloc_get_gint_stats(kmalloc_get_arena("_uram"));

#ifdef FX9860G
    s2 = kmalloc_get_gint_stats(kmalloc_get_arena("pram0"));
    pe_debug_printf("%s: _uram[used=%d free=%d] pram0[used=%d free=%d]\n",
        prefix,
        s1->used_memory, s1->free_memory,
        s2->used_memory, s2->free_memory);
#endif

#ifdef FXCG50
    s2 = kmalloc_get_gint_stats(kmalloc_get_arena("_ostk"));
    pe_debug_printf("%s: _uram[used=%d free=%d] _ostk[used=%d free=%d]\n",
        prefix,
        s1->used_memory, s1->free_memory,
        s2->used_memory, s2->free_memory);
#endif
}

void pe_debug_screenshot(void)
{
    usb_open_wait();
    usb_fxlink_screenshot(true);
}

void pe_debug_toggle_videocapture(void)
{
    videocapture = !videocapture;
}

void pe_debug_run_videocapture(void)
{
    if(videocapture) {
        usb_open_wait();
        usb_fxlink_videocapture(true);
    }
}

void pe_debug_close(void)
{
    usb_close();
}

#endif /* PE_DEBUG */
