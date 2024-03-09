//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.resources: Utility for allocating auto-freed resources in Python code
//
// This header provides wrapper around gint or other functions that allocate
// resources. Its purpose is to track these resources and allow PythonExtra to
// free them automatically when the execution of Python program finishes. This
// avoids any situation where important resources are exhausted over the course
// of multiple executions.
//
// This header only provides wrappers around allocation and deallocation
// functions; other usual functions can be used directly. Each allocation
// function has an extra boolean parameter indicating whether to raise a
// warning if debug mode is enabled and the resource is auto-freed. It should
// be set to true when there is a mechanism to free the resource, false when
// there isn't and auto-free is the expected outcome.
//---

#include <gint/timer.h>

/* Autofree all resources. */
void pe_resources_autofree(void);

/* gint's timer_configure() and timer_stop() */
int pe_timer_configure(int, uint64_t, gint_call_t, bool warn_autofree);
void pe_timer_stop(int);
