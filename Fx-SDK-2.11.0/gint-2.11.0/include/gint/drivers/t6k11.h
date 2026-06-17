//---
//	gint:drivers:t6k11 - Toshiba T6K11 driver
//
//	This is the screen driver used by fx9860g (monochrome) models.
//---

#ifndef GINT_DRIVERS_T6K11
#define GINT_DRIVERS_T6K11

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

/* t6k11_display() - send vram data to the LCD device

   A typical 128*64 VRAM area would use y1 = 0, y2 = 64, stride = 16. It is
   possible to send only a section of the video RAM by specifying y1 > 0 or
   y2 < 64 and moving the vram pointer accordingly.

   @vram    Video RAM address
   @y1      First row to send
   @y2      Last row to send + 1
   @stride  Number of bytes between each row */
void t6k11_display(const void *vram, int y1, int y2, size_t stride);

/* t6k11_contrast() - change the contrast setting

   Adjusts the screen contrast. The parameter takes value in range 0 .. 32 and
   is adjusted when not in range.

     0 (bright)  <-------- 14 (OS default) --------> 32 (dark)

   It is not possible to read the contrast value from the display driver, but
   the system stores its contrast setting in RAM. The location is OS-dependent.
   It would be possible to restore contrast, or update the system value on
   change, if the address was known for all OS versions.

     OS 02.05.2201	8800'b93c

   @contrast  Requested contrast value */
void t6k11_contrast(int contrast);

/* t6k11_backlight() - manage the screen backlight

   Changes the backlight status depending on the value of the argument:
   - If setting > 0, turns the backlight on.
   - If setting = 0, turns the backlight off.
   - If setting < 0, toggles backlight.

   This function has no effect on models that do not support the backlight,
   although gint does not provide any facility for detecting them.

   @setting  Requested backlight setting */
void t6k11_backlight(int setting);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_T6K11 */
