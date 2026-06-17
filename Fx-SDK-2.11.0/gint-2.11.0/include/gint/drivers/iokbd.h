//---
//	gint:drivers:iokbd - I/O ports-driven keyboard scanner
//
//	This is for SH3 only. It reads key presses from ports A/B/M.
//---

#ifndef GINT_DRIVERS_IOKBD
#define GINT_DRIVERS_IOKBD

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

/* iokbd_scan() - scan ports A/B/M to generate 12 rows of key data
   Numbering of rows is consistent with the keyboard.
   @scan  12-byte buffer filled with row data */
void iokbd_scan(uint8_t *scan);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_IOKBD */
