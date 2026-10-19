//---
//	gint:intc - Interrupt Controller
//---

#ifndef GINT_INTC
#define GINT_INTC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/defs/call.h>

//---
//	Interrupt names
//---

/* Because of SH3/SH4 differences and the different ways interrupts are mapped
   to bits of IPR and IMR registers, gint defines the following names for
   interrupt signals, and their corresponding IPR and IMR bits are defined in
   the INTC driver. */
enum {
	/* Timer Unit [TMU] */
	INTC_TMU_TUNI0,
	INTC_TMU_TUNI1,
	INTC_TMU_TUNI2,
	/* Extra Timer Unit [TMU]; SH7305-only except for ETMU_TUNI0 */
	INTC_ETMU_TUNI0,
	INTC_ETMU_TUNI1,
	INTC_ETMU_TUNI2,
	INTC_ETMU_TUNI3,
	INTC_ETMU_TUNI4,
	INTC_ETMU_TUNI5,
	/* DMA Controller [DMA0]; a single IPR is used for DEI0..3, and another
	   IPR is used for DEI4..5 and DADERR together */
	INTC_DMA_DEI0,
	INTC_DMA_DEI1,
	INTC_DMA_DEI2,
	INTC_DMA_DEI3,
	INTC_DMA_DEI4,
	INTC_DMA_DEI5,
	INTC_DMA_DADERR,
	/* Serial Communication Interface with FIFO (SCIF0) */
	INTC_SCIF0,
	/* Real-Time Clock [RTC]; a single IPR covers all 3 interrupts */
	INTC_RTC_ATI,
	INTC_RTC_PRI,
	INTC_RTC_CUI,
	/* SPU; interrupts from the DSPs and the SPU-bound DMA */
	INTC_SPU_DSP0,
	INTC_SPU_DSP1,
	/* USB communication */
	INTC_USB,
};

//---
//	Interrupt control functions
//---

/* intc_priority(): Configure the level of interrupts

   This function changes the interrupt level of the requested interrupt. Make
   sure you are aware of interrupt assignments to avoid breaking other code.
   This function is mainly used by drivers to enable the interrupts that they
   support.

   The interrupt level should be in the range 0 (disabled) .. 15 (highest
   priority). On SH7305, if the level is not 0, this function also clears the
   associated interrupt mask.

   @intname  Name of the targeted interrupt, from the enumeration above
   @level    Requested interrupt level
   Returns the interrupt level that was assigned before the call. */
int intc_priority(int intname, int level);

/* intc_handler(): Install interrupt handlers

   This function installs (copies) interrupt handlers in the VBR space of the
   application. Each handler is a 32-byte block aligned on a 32-byte boundary.
   When an interrupt request is accepted, the hardware jumps to a specific
   interrupt handler at an address that depends on the interrupt source.

   For safety, interrupt handlers should avoid referring to data from other
   blocks because the arrangement of blocks at runtime depends on event codes.
   The assembler program will assume that consecutive blocks in the source code
   will be consecutive in memory, which is not always true. Avoiding cross-
   references is a practical rule to avoid problems. (gint breaks this rule
   quite often but does it safely.)

   This function allows anyone to replace any interrupt handler so make sure
   you're not interfering with interrupt assignments from gint or a library.

   The first parameter event_code represents the event code associated with the
   interrupt. If it's not a multiple of 0x20 then you're doing something wrong.
   The codes are normally platform-dependent, but gint always uses SH7305
   codes. SH3 platforms have a different, compact VBR layout. gint_inthandler()
   translates the provided SH7305 codes to the compact layout and the interrupt
   handler translates the hardware SH3 codes to the compact layout as well. See
   gint's source in <src/kernel/inth.S> and <src/intc/intc.c>. Please note that
   intc_handler() uses a table that must be modified for every new SH3
   interrupt code to extend the compact scheme.

   The handler function is run in the kernel register bank with interrupts
   disabled and must end with 'rts' (not 'rte') as the main interrupt handler
   saves some registers for you. By default, user bank registers are not saved
   except for gbr/mach/macl; if you want to call back to arbitrary code safely,
   use gint_inth_callback() in your handler.

   For convenience gint allows any block size to be loaded as an interrupt
   handler, but it should really be a multiple of 0x20 bytes and not override
   other handlers. If it's not written in assembler, then you're likely doing
   something wrong. Using __attribute__((interrupt_handler)), which uses rte,
   is especially wrong.

   It is common for interrupt handlers to have a few bytes of data, such as the
   address of a callback function. gint often stores this data in the last
   bytes of the block. This function returns the VBR address of the block which
   has just been installed, to allow the caller to edit the parameters later.

   @event_code  Identifier of the interrupt block
   @handler     Address of handler function
   @size        How many bytes to copy
   Returns the VBR address where the handler was installed. */
void *intc_handler(int event_code, void const *handler, size_t size);

/* intc_handler_function(): Install a function as an interrupt handler

   This function can be used to install simple interrupt handlers. It installs
   a pre-written interrupt handler that performs the provided indirect call.
   Essentially it means that the interrupt handler can be written in C without
   the numerous constraints of intc_handler(), at the cost of always going back
   to userspace and a small time overhead.

   Since gint_inth_callback is used to do the heavy lifting of setting up a sane
   context for C code, the gint_call_t object can be built with GINT_CALL_FLAG
   if the called function expects a gint_inth_callback_context_t structure as its
   first argument.

   @event_code  Identifier of the interrupt block
   @function    Function to use as a handler
   Returns true on success, false if the event code is invalid. */
bool intc_handler_function(int event_code, gint_call_t function);

#ifdef __cplusplus
}
#endif

#endif /* GINT_INTC */
