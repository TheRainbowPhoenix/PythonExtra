//---
//	gint:drivers:r61524 - Reneses R61524 driver
//
//	This driver is used to control the 16-bit color LCD of the Prizm and
//	fx-CG 50 series.
//---

#ifndef GINT_DRIVERS_R61524
#define GINT_DRIVERS_R61524

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

enum {
	/* Send data through the DMA, return early (triple-buffering) */
	R61524_DMA,
	/* Send data through DMA, wait to return (no interrupts) */
	R61524_DMA_WAIT,
	/* Send data through CPU (slow!) */
	R61524_CPU,
};

/* r61524_display(): Send an image to the display

   This function sends [height] lines of the provided [vram] starting from line
   [start] and going down 396 pixels each line. Three methods are avaiable, the
   default is to use R61524_DMA which is what you almost always want.

   @vram    Source VRAM with a stride of 396*2 bytes
   @start   First line to send
   @height  Number of lines to send
   @method  Transfer method, see above */
void r61524_display(uint16_t *vram, int start, int height, int method);

/* r61524_display_rect(): Send a rectangular section of VRAM to the display

   This function updates the rectangle going from (xmin,ymin) to (xmax,ymax) in
   the VRAM to the display (both ends included). This can be faster than a full
   dupdate() or a striped r61524_display() depending on the situation. However,
   because the source VRAM is not contiguous, the transfer is only possible by
   CPU, so this is only interesting for small regions.

   @vram        Source VRAM with a stride of 396*2 bytes
   @xmin @xmax  Horizontal range to be updated (both included)
   @ymin @ymax  Vertical range to be updated (both included) */
void r61524_display_rect(uint16_t *vram, int xmin, int xmax, int ymin,
   int ymax);

/* r61524_display_mono_128x64(): Display a mono-style VRAM
   This experimental function updates the display with the contents of a 128x64
   VRAM, used with the fxg3a compilation target.
   TODO: Make that a video mode. */
void r61524_display_mono_128x64(uint32_t *vram);

/* r61524_display_mono_128x64(): Display a gray-style VRAM
   This experimental function updates the display with the contents of a 128x64
   gray VRAM pair, used with the fxg3a compilation target.
   TODO: Make that a video mode. */
void r61524_display_gray_128x64(uint32_t *light, uint32_t *dark);

/* r61524_start_frame(): Prepare the display for a region update

   This function sets up the display driver to receive graphics data to update
   the rectangle going from (xmin,ymin) to (xmax,ymax) (both included). This is
   the initial step of r61524_display(), which is normally followed by writing
   all the data to 0xb4000000.

   In order to write with the DMA, it is necessary to write the full horizontal
   range and select ymin and ymax to be congruent to 0 and 3 modulo 4.

   This function can be used to implement additional display driver update
   methods or alternate rendering pipelines. */
void r61524_start_frame(int xmin, int xmax, int ymin, int ymax);

/* r162524_win_get() and r61524_win_set(): Manipulate the display window

   These functions change the screen rectangle where data is shown. Normally
   gint uses the full screen of 396x224. The system uses a subrectangle of
   384x216.

   These functions don't integrate nicely with gint's drawing API, so if you
   want to use them make sure you know how <gint/display.h> is going to be
   impacted. */
void r61524_win_get(uint16_t *HSA, uint16_t *HEA, uint16_t *VSA,uint16_t *VEA);
void r61524_win_set(uint16_t HSA, uint16_t HEA, uint16_t VSA, uint16_t VEA);

//---
// Low-level functions
//---

/* r61524_get(): Read the value of an R61524 register
   This is provided for testing and if you know what you're doing. */
uint16_t r61524_get(int ID);

/* r61524_set(): Write the value of an R61524 register
   This is provided for testing and if you know what you're doing. */
void r61524_set(int ID, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_R61524 */
