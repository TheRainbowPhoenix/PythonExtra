//---
// gint:gdb - GDB remote serial protocol
//---

#ifndef GINT_GDB
#define GINT_GDB

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* gdb_cpu_state_t: State of the CPU when breaking
   This struct keep the same register indices as those declared by GDB to allow
   easy R/W without needing a "translation" table.
   See : https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=gdb/sh-tdep.c;
         h=c402961b80a0b4589243023ea5362d43f644a9ec;hb=4f3e26ac6ee31f7bc4b04abd
         8bdb944e7f1fc5d2#l361
 */
// TODO : Should we expose r*b*, ssr, spc ? are they double-saved when breaking
//        inside an interrupt handler ?
typedef struct {
	union {
		struct {
			uint32_t r0;
			uint32_t r1;
			uint32_t r2;
			uint32_t r3;
			uint32_t r4;
			uint32_t r5;
			uint32_t r6;
			uint32_t r7;
			uint32_t r8;
			uint32_t r9;
			uint32_t r10;
			uint32_t r11;
			uint32_t r12;
			uint32_t r13;
			uint32_t r14;
			uint32_t r15;
			uint32_t pc;
			uint32_t pr;
			uint32_t gbr;
			uint32_t vbr;
			uint32_t mach;
			uint32_t macl;
			uint32_t sr;
		} reg;
		uint32_t regs[23];
	};
} gdb_cpu_state_t;

/* gdb_start(): Start the GDB remote serial protocol server

   This function starts the GDB stub (program that communicates with GDB). It
   takes over USB, and returns once communication is established with a GDB
   instance on the computer. Normally this is called by the panic handler after
   a crash. It can also be called manually, in which case it should be followed
   by a call to gdb_main() with the current state (NULL if you don't have any).

   If the GDB stub is already started, this is a no-op. Returns 0 on success
   and a negative value if the USB setup fails. */
int gdb_start(void);

/* gdb_main(): Main GDB loop
   Main loop for when the program is in a paused state. Communicates with GDB
   over USB and mutates the cpu_state struct, memory and the UBC configuration
   accordingly. Returns once the program should resume. */
void gdb_main(gdb_cpu_state_t *cpu_state);

/* gdb_start_on_exception(): Set the GDB stub to autostart on crashes
   This function sets a crash handler that starts the GDB stub whenever a
   System ERROR occurs. */
// TODO: Get some visible signal on-screen when that happens
void gdb_start_on_exception(void);

/* gdb_redirect_streams(): Select whether to redirect stdout/stderr

   This function specifies whether stdout and stderr shall be redirected when
   debugging. If redirected, stdout and stderr will change from their default
   implementation (or the one supplied by the user via open_generic()) to a
   stubcall that prints text in the GDB console. This setting must be set
   before GDB starts, so usually just after gdb_start_on_exception().

   This is intended to be used with debug methods that print text. For example,
   if the program has a status() function that prints information useful when
   debugging, one might want to invoke it from GDB with "call status()". With
   default stdout/stderr this would be at best impractical to read the output
   on the calculator, at worst buggy due to the program being interrupted.
   Redirecting stdout/stderr allows the result to be printed in GDB.

   The default is to not redirect stdout/stderr. */
void gdb_redirect_streams(bool redirect_stdout, bool redirect_stderr);

/** Stubcalls **/

/* Write to a file descriptor on the remote debugger. */
void gdb_stubcall_write(int fd, void const *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GINT_GDB */
