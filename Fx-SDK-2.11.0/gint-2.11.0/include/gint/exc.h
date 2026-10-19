//---
//	gint:exc - Exception handling
//
//	This small module is used to display exceptions and configure when the
//	exception handler displays these messages. This is for advanced users
//	only!
//---

#ifndef GINT_EXC
#define GINT_EXC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <stdint.h>

/* gint_panic(): Panic handler function
   This function is called when an uncaught CPU exception is generated. By
   default, it displays a full-screen error message with the event code and
   basic debugging information. Some custom event codes may be used for kernel
   failure scenarios. This function never returns. */
GNORETURN void gint_panic(uint32_t code);

/* gint_panic_set(): Change the panic handler function
   Sets up a different panic function instead of the default. It the argument
   is NULL, restores the default. */
void gint_panic_set(GNORETURN void (*panic)(uint32_t code));

/* gint_exc_catch(): Set a function to catch exceptions

   Sets up an exception-catching function. If an exception occurs, before a
   panic is raised, the exception-catching function is executed with interrupt
   disabled and is given a chance to handle the exception. Passing NULL
   disables this feature.

   The exception-catching function can do anything that does not use interrupts
   or cause an exception, such as logging the exception or any other useful
   mechanism. TLB misses are enabled. In general, this function should be as
   short as possible.

   What happens next depends on the return value:
   * If it returns 0, the exception is considered handled and execution
     continues normally at or after the offending instruction.
   * If it returns nonzero, a panic is raised.

   Please be aware that many exceptions are of re-execution type. When
   execution restarts after such an exception is handled, the offending
   instruction is re-executed. This can cause the exception handling mechanism
   to loop. Use gint_exc_skip() to skip over the offending instruction when
   needed. Whether an exception is of re-execution type depends on the
   exception code. */
void gint_exc_catch(int (*handler)(uint32_t code));

/* gint_exc_skip(): Skip pending exception instructions

   Many exceptions re-execute the offending instruction after the exception is
   handled. For instance the TLB miss handler is supposed to load the required
   page into memory, so that the instruction that accessed unmapped memory can
   be successfully re-executed.

   When an exception-catching function records an exception without solving it,
   this re-execution will fail again and the exception handling process will
   loop. In such a situation, gint_exc_skip() can be used to manually skip the
   offending instruction, if this is an acceptable resolution.

   @instructions  Number of instructions to skip (usually only one) */
void gint_exc_skip(int instructions);

#ifdef __cplusplus
}
#endif

#endif /* GINT_EXC */
