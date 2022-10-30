//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "shell.h"
#include <gint/timer.h>
#include <gint/display.h>

/* The global terminal. */
console_t *pe_shell_console;
/* Timer handle for the refresh clock. */
static int pe_shell_timer = -1;
/* Whether there is new shell data to be shown on the next frame. */
static bool pe_shell_update = true;

static void pe_shell_draw(void)
{
    dclear(C_WHITE);

#ifdef FX9860G
    int rows = 8;
    console_render(1, 1, pe_shell_console, DWIDTH-2, rows);
#else
    dprint(3, 3, C_BLACK, "PythonExtra, very much WIP :)");
    dline(2, 16, DWIDTH-3, 16, C_BLACK);
    int rows = 12;
    console_render(3, 20, pe_shell_console, DWIDTH-6, rows);
    int y = 20 + PE_CONSOLE_LINE_SPACING * rows;
    dline(2, y, DWIDTH-3, y, C_BLACK);
#endif

    dupdate();
}

static int pe_shell_timer_handler(void)
{
    if(pe_shell_update) {
        pe_shell_draw();
        pe_shell_update = false;
    }
    return TIMER_CONTINUE;
}

void pe_shell_schedule_update(void)
{
    pe_shell_update = true;
}

void pe_shell_graphics_mode(void)
{
    pe_shell_update = false;
}

void pe_shell_init(void)
{
    pe_shell_console = console_create(8192);

    pe_shell_timer = timer_configure(TIMER_ANY, 1000000 / PE_SHELL_FPS,
        GINT_CALL(pe_shell_timer_handler));
    timer_start(pe_shell_timer);
}

void pe_shell_deinit(void)
{
    timer_stop(pe_shell_timer);
    console_destroy(pe_shell_console);
}
