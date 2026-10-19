//---
// gint:ubc - User Break Controller driver
//---

#ifndef GINT_UBC
#define GINT_UBC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/gdb.h>

/* Read and write the DBR register */
void ubc_setDBR(void* DBR);
void* ubc_getDBR(void);

/* ubc_dbh(): Low level UBC debug handler
   The handler will backup the current CPU state and call ubc_debug_handler(). */
void ubc_dbh(void);
/* ubc_debug_handler(): C level UBC debug handler
   The execution will be redirected to the handler set by the user or the break
   will be ignored in case no handler has been set. */
void ubc_debug_handler(gdb_cpu_state_t* cpu_state);
/* ubc_set_debug_handler(): Set user debug handler
   Set a custom debug handler that will be called when a break condition from
   the UBC is reached. */
void ubc_set_debug_handler(void (*h)(gdb_cpu_state_t*));

/* ubc_dbh_lock: Lock set by ubc_dbh() when a UBC break is currently being
   handled. */
extern uint8_t ubc_dbh_lock;

/* UBC Breakpoint types */
typedef enum {
	UBC_BREAK_BEFORE, /* Break before the instruction is executed */
	UBC_BREAK_AFTER,  /* Break after the instruction is executed :
                             at this point PC will point to the next instruction. */
} ubc_break_mode_t;

/* ubc_set_breakpoint(): Set a breakpoint in a UBC channel and enable it
   Return false when an invalid channel number is provided, true if the
   breakpoint was correctly set up. */
bool ubc_set_breakpoint(int channel, void* break_address, ubc_break_mode_t break_mode);
/* ubc_get_break_address(): Get a breakpoint address if it's enabled
   If the channel is disabled the function will return false and *break_address
   will not be updated. */
bool ubc_get_break_address(int channel, void** break_address);
/* ubc_disable_channel(): Disable a UBC channel
   Return true on success. If an invalid channel number is provided, it will
   return false. */
bool ubc_disable_channel(int channel);

#ifdef __cplusplus
}
#endif

#endif /* GINT_UBC */
