//---
//	gint:display-fx - fx9860g drawing functions
//
//	This module is in charge of all monochrome rendering. The gray engine
//	has its own functions, but often relies on this module (because the
//	gray effect is created through two monochrome buffers).
//---

#ifndef GINT_DISPLAY_FX
#define GINT_DISPLAY_FX

#if GINT_RENDER_MONO

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

/* Dimensions of the VRAM */
#define DWIDTH 128
#define DHEIGHT 64

/* gint VRAM address. This value must always point to a 4-aligned buffer of
   size 1024. Any function can use it freely to:
   - Use another video ram area (triple buffering or more, gray engine);
   - Implement additional drawing functions;
   - Store data when not drawing. */
extern uint32_t *gint_vram;

/* color_t - colors available for drawing
   The following colors are defined by the library:

     OPAQUE COLORS (override existing pixels)
       white, black	- the usual thing
       light, dark	- intermediate colors used with the gray engine

     OPERATORS (combine with existing pixels)
       none		- leaves unchanged
       invert		- inverts white <-> black, light <-> dark
       lighten		- shifts black -> dark -> light -> white -> white
       darken		- shifts white -> light -> dark -> black -> black

   Not all colors can be used with all functions. To avoid ambiguities, all
   functions explicitly indicate compatible colors. */
typedef enum
{
	/* Opaque colors */
	C_WHITE    = 0,
	C_LIGHT    = 1,
	C_DARK     = 2,
	C_BLACK    = 3,

	/* Monochrome operators */
	C_NONE     = 4,
	C_INVERT   = 5,

	/* Gray operators */
	C_LIGHTEN  = 6,
	C_DARKEN   = 7,

} color_t;

//---
//	Image rendering (bopti)
//---

/* bopti_image_t - image files encoded for bopti
   This format is the result of encoding images for bopti with the fxSDK's
   [fxconv] tool. The bopti routines can render it extremely fast, which makes
   it preferable over plain bitmaps if the images are never edited. */
typedef struct
{
	/* Image can only be rendered with the gray engine */
	uint gray	:1;
	/* Left for future use */
	uint		:3;
	/* Image profile (uniquely identifies a rendering function) */
	uint profile	:4;
	/* Full width, in pixels */
	uint width	:12;
	/* Full height, in pixels */
	uint height	:12;

	/* Raw layer data */
	uint8_t *data;

} GPACKED(4) bopti_image_t;

/* Image formats ("profiles") */
enum {
	/* MONO: black/white, 1 layer (bw) */
	IMAGE_MONO = 0,
	/* MONO_ALPHA: black/white/transparent, 2 layers (alpha+bw) */
	IMAGE_MONO_ALPHA = 1,
	/* GRAY: black/dark/light/white, 2 layers (light+dark) */
	IMAGE_GRAY = 2,
	/* GRAY_ALPHA: black/dark/light/white/transparent, 3 layres
	   (alpha+light+dark) */
	IMAGE_GRAY_ALPHA = 3,
};

/* Number of layers in the image. */
GINLINE static int image_layer_count(int profile)
{
	return profile + (profile <= 1);
}

#ifdef __cplusplus
}
#endif

#endif /* GINT_RENDER_MONO */

#endif /* GINT_DISPLAY_FX */
