//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.shell: Shell integration
//
// This header covers application-integration aspects of the shell. For the
// implementation of the console itself (input, escape sequences, etc). see
// <console.h> instead.
//
// The shell consists of a terminal-emulating `console_t` along with some input
// UI. During execution, the shell is redrawn only (1) when new data is printed
// or requested, and (2) a fixed refresh timer hits. Condition 1 avoids
// needless updates for non-printing programs, while condition 2 prevents
// performance issues for sequences that print one character at a time (which
// is quite common even within MicroPython builtins).
//
// If the built-in function gint.dupdate() is used, the shell also hands over
// display updates to "graphics mode", where only manual dupdate() calls push
// the VRAM. Switching to graphics mode cancels any scheduled shell updates.
// Graphics mode is exited once a new shell update is explicity scheduled,
// typically by a call to print() or input().
//---

#ifndef __PYTHONEXTRA_SHELL_H
#define __PYTHONEXTRA_SHELL_H

#include "console.h"

/* Shell frame frequency, ie. cap on the number of shell redraws per second. */
#define PE_SHELL_FPS 30

/* The terminal. */
extern console_t *pe_shell_console;

/* Schedule a redraw of the shell interface at the next shell frame. This is
   called whenever data is printed into the shell or input() is used. The
   scheduled redraw might be canceled by switching to graphics mode before the
   frame timer fires. */
void pe_shell_schedule_update(void);

/* Switch to graphics mode. This cancels pending shell updates. */
void pe_shell_graphics_mode(void);

//=== Internal functions ===//

void pe_shell_init(void);
void pe_shell_deinit(void);

#endif /* __PYTHONEXTRA_SHELL_H */
