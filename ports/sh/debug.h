//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.debug: Debugging utilities
//
// Most of the debugging occurs via USB. This module also includes screenshots
// and other developer-only utilities. The interface always exists but it's
// almost entirely no-oped if PE_DEBUG isn't set.
//---

#ifndef __PYTHONEXTRA_DEBUG_H
#define __PYTHONEXTRA_DEBUG_H

/* PE_DEBUG is set in mpconfigport.h. */
#include "mpconfigport.h"

/* Initialize debugging resources (mostly the USB connection). */
void pe_debug_init(void);

/* Panic with a message (available even if PE_DEBUG=0). */
void pe_debug_panic(char const *msg)
__attribute__((noreturn));

/*** Memory watch utilities (enabled even when PE_DEBUG=0) ***/

#ifdef FX9860G
struct pe_debug_meminfo {
    uint16_t _uram_used, _uram_free;
    uint32_t pram0_used, pram0_free;
};
#endif

#ifdef FXCG50
struct pe_debug_meminfo {
    uint16_t _uram_used, _uram_free;
    uint32_t _ostk_used, _ostk_free;
};
#endif

enum {
   /* Just after entering main() */
   PE_DEBUG_STARTUP_MAIN,
   /* After allocating console */
   PE_DEBUG_STARTUP_CONSOLE,
   /* After initializing MicroPython */
   PE_DEBUG_STARTUP_UPY,
   /* After printing the first prompt */
   PE_DEBUG_STARTUP_PROMPT,
   /* After allocating and initializing the GUI */
   PE_DEBUG_STARTUP_UI,

   PE_DEBUG_STARTUP_N,
};

/* Fetch data about current memory statistics. */
void pe_debug_get_meminfo(struct pe_debug_meminfo *info);

/* Buffer for storing meminfo during startup. */
extern struct pe_debug_meminfo pe_debug_startup_meminfo[PE_DEBUG_STARTUP_N];

#define pe_debug_get_startup_meminfo(NAME) \
   pe_debug_get_meminfo(&pe_debug_startup_meminfo[PE_DEBUG_STARTUP_ ## NAME])

/* Browse memory info through a GUI. */
void pe_debug_browse_meminfo(void);

/*** Debugging functions enabled only when PE_DEBUG=1 ***/

/* Print to the debug stream. This function is also called DEBUG_printf in
   MicroPython code. */
int pe_debug_printf(char const *fmt, ...);

/* Print information about allocation status. */
void pe_debug_kmalloc(char const *prefix);

/* Take a screenshot. */
void pe_debug_screenshot(void);

/* Toggle video capture. */
void pe_debug_toggle_videocapture(void);

/* Send a video capture frame if video capture is enabled. */
void pe_debug_run_videocapture(void);

/* Close the debugging ressources */
void pe_debug_close(void);

#if !PE_DEBUG
#define PE_DEBUG_NOOP do {} while(0)
#define pe_debug_init(...)                 PE_DEBUG_NOOP
#define pe_debug_printf(...)               PE_DEBUG_NOOP
#define pe_debug_kmalloc(...)              PE_DEBUG_NOOP
#define pe_debug_screenshot(...)           PE_DEBUG_NOOP
#define pe_debug_toggle_videocapture(...)  PE_DEBUG_NOOP
#define pe_debug_run_videocapture(...)     PE_DEBUG_NOOP
#define pe_debug_close(...)                PE_DEBUG_NOOP
#endif

#endif /* __PYTHONEXTRA_DEBUG_H */
