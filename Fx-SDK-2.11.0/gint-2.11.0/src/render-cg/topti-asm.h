//---
//	gint:render-cg:topti-asm - Assembler drawing routines for topti
//---

#ifndef GINT_RENDERCG_TOPTIASM
#define GINT_RENDERCG_TOPTIASM

#include <gint/config.h>
#if GINT_RENDER_RGB

/* Text rendering functions

   @vram    Pointer to VRAM, offset for subglyph position
   @data    Glyph data, offset for subglyph position
   @color   topti_glyph_fg:    Foreground color
            topti_glyph_bg:    Background color
            topti_glyph_fg_bg: (fg << 16) | bg
   @height  Subglyph height
   @width   Sublgyph width
   @stride  Storage width of subglyph - width
   @index   Starting index in data, ie. top * storage width + left */
typedef void asm_text_t(uint16_t *vram, uint32_t const * data, uint32_t color,
	int height, int width, int stride, int index);

/* Opaque foreground, transparent background */
extern asm_text_t topti_glyph_fg;
/* Transparent foreground, opaque background */
extern asm_text_t topti_glyph_bg;
/* Opaque foreground, opaque background */
extern asm_text_t topti_glyph_fg_bg;

#endif

#endif /* GINT_RENDERFX_TOPTIASM */
