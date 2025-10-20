//---
//  gint:drivers:r61523 - Reneses R61523 driver
//
//  This driver is used to control the 16-bit color LCD of the fx-CP 400.
//---

#ifndef GINT_DRIVERS_R61523
#define GINT_DRIVERS_R61523

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

/* r61523_display(): Update the entire display (320x528) */
void r61523_display(uint16_t *vram);

/* r61523_win_set(): Set the display window */
void r61523_win_set(int x1, int x2, int y1, int y2);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_R61523 */
