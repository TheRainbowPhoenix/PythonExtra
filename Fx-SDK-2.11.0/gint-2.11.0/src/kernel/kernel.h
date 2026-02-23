//---
//	core:kernel - Kernel functions
//---

#ifndef GINT_CORE_KERNEL
#define GINT_CORE_KERNEL

/* gint_load_onchip_sections(): Initialize on-chip memory sections */
void gint_load_onchip_sections(void);

/* gint_copy_vram(): Copy gint's VRAM to the OS to avoid flickering during
   certain world switches. */
void gint_copy_vram(void);

/* kinit(): Install and start gint */
void kinit(void);

/* kquit(): Quit gint and give back control to the system */
void kquit(void);

#endif /* GINT_CORE_KERNEL */
