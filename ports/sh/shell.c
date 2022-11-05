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

/* TODO: Port graphics mode over to the JustUI setup */
void pe_shell_graphics_mode(void)
{
    // pe_shell_update = false;
}
