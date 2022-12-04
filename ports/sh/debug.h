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

/* Print to the debug stream. This function is also called DEBUG_printf in
   MicroPython code. */
int pe_debug_printf(char const *fmt, ...);

/* Print information about allocation status. */
void pe_debug_kmalloc(void);

/* Take a screenshot. */
void pe_debug_screenshot(void);

#if !PE_DEBUG
#define PE_DEBUG_NOOP do {} while(0)
#define pe_debug_init(...)          PE_DEBUG_NOOP
#define pe_debug_printf(...)        PE_DEBUG_NOOP
#define pe_debug_kmalloc(...)       PE_DEBUG_NOOP
#define pe_debug_screenshot(...)    PE_DEBUG_NOOP
#endif

#endif /* __PYTHONEXTRA_DEBUG_H */
