//---
//	render-fx - Internal definitions for the display module on fx9860g
//---

#ifndef RENDER_FX
#define RENDER_FX

#include <gint/defs/types.h>
#include <gint/defs/attributes.h>
#include <gint/display.h>
#include <gint/config.h>

#if GINT_RENDER_MONO

struct rbox;

/* masks(): Compute the vram masks for a given rectangle

   Since the VRAM is line-based with four uin32_t elements per row, we can
   execute any operation on a rectangle by running it on each set of four
   uint32_t elements.

   This function calculates four uint32_t values and stores them in @mask. Each
   of the 128 bits in this array represents a column of the screen, and the bit
   of column c is 1 iff x1 <= c <= x2.

   These masks can then be and-ed/or-ed/anything on the VRAM to draw.

   @x1 @x2  Targeted screen range, horizontally (both included)
   @masks   Stores the result of the function (four uint32_t values) */
void masks(int x1, int x2, uint32_t *masks);

/* bopti_clip(): Clip a bounding box to image and VRAM
   @img   Image encoded by [fxconv]
   @rbox  Rendering box */
int bopti_clip(bopti_image_t const *img, struct rbox *rbox);

/* bopti_render(): Render a bopti image
   Copies an image into the VRAM. This function does not perform clipping;
   use bopti_clip() on the rbox before calling it if needed.

   @img     Image encoded by [fxconv]
   @rbox    Rendering box (may or may not be clipped)
   @v1 @v2  VRAMs (gray rendering is used if v2 != NULL) */
void bopti_render(bopti_image_t const *img, struct rbox *rbox, uint32_t *v1,
    uint32_t *v2);

/* bopti_render_scsp(): Single-column single-position image renderer
   This function is a specialized version of bopti_render() that can be used
   when only a single column of the source image is used (all pixels to be
   rendered are in a single 32-aligned 32-wide pixel column of the source) and
   a single position of the VRAM is used (all pixels to be rendered end up in a
   single 32-aligned 32-wide pixel column of the VRAM). */
void bopti_render_scsp(bopti_image_t const *img, struct rbox *rbox,
    uint32_t *v1, uint32_t *v2);

//---
// Image rendering
//---

/* pair_t: A pair of consecutive VRAM longwords */
typedef struct {
   uint32_t l;
   uint32_t r;
} pair_t;

/* quadr_t: Two pairs for light and gray VRAMs */
typedef struct {
   uint32_t l1;
   uint32_t r1;
   uint32_t l2;
   uint32_t r2;
} quadr_t;

/* Signature of mono rendering functions */
typedef pair_t asm_mono_t(pair_t p,  void **layer, uint32_t *masks, int x);
/* Signature of gray rendering functions */
typedef void asm_gray_t(quadr_t q, void **layer, uint32_t *masks, int x,
   quadr_t *ret);
/* Signature of mono single-column single-position rendering functions */
typedef void asm_mono_scsp_t(uint32_t *vram, uint32_t const *layer,
   uint32_t mask, int x);
/* Signature of gray single-column single-position rendering functions */
typedef void asm_gray_scsp_t(uint32_t *v1, uint32_t const *layer,
   uint32_t mask, uint32_t *v2, int x);

/* Type of any rendering function */
typedef union {
   void *gen;
   asm_mono_t *asm_mono;
   asm_gray_t *asm_gray;
   asm_mono_scsp_t *asm_mono_scsp;
   asm_gray_scsp_t *asm_gray_scsp;
} bopti_asm_t;

/* Each of the following rendering functions:
   1. Takes VRAM data for two longword positions of the screen.
   2. Reads data for one longword position of the image from *layer. This
      consists in n longwords where n is the number of layers in the image.
   3. Increments *layer by 4*n.
   4. Shifts the image data and apply it to the VRAM positions in accordance
      with the two masks given in the masks argument. */

/* bopti_asm_mono(): Rendering function for the "mono" profile */
extern asm_mono_t bopti_asm_mono;
/* bopti_asm_mono_alpha(): Rendering function for the "mono alpha" profile */
extern asm_mono_t bopti_asm_mono_alpha;

/* bopti_gasm_mono(): "mono" profile on gray VRAMs */
extern asm_gray_t bopti_gasm_mono;
/* bopti_gasm_mono_alpha(): "mono_alpha" profile on gray VRAMs */
extern asm_gray_t bopti_gasm_mono_alpha;
/* bopti_asm_gray(): Rendering function for the "gray" profile */
extern asm_gray_t bopti_gasm_gray;
/* bpoti_asm_gray_alpha(): Rendering function for the "gray_alpha" profile */
extern asm_gray_t bopti_gasm_gray_alpha;

/* Each of the following rendering functions:
   1. Takes VRAM data from one longword position of the screen.
   2. Reads data from one longword position of the image from layer.
   3. Shifts the image data and applies it to the VRAM position.
   None update the layer pointer. */

/* bopti_asm_mono_scsp(): SCSP "mono" profile */
extern asm_mono_scsp_t bopti_asm_mono_scsp;
/* bopti_asm_mono_alpha_scsp(): SCSP "mono_alpha" profile */
extern asm_mono_scsp_t bopti_asm_mono_alpha_scsp;

/* bopti_gasm_mono_scsp(): SCSP "mono" profile on gray VRAMs */
extern asm_gray_scsp_t bopti_gasm_mono_scsp;
/* bopti_gasm_mono_scsp_alpha(): SCSP "mono_alpha" profile on gray VRAMs */
extern asm_gray_scsp_t bopti_gasm_mono_alpha_scsp;
/* bopti_asm_gray_scsp(): SCSP "gray" profile */
extern asm_gray_scsp_t bopti_gasm_gray_scsp;
/* bpoti_asm_gray_alpha_scsp(): SCSP "gray_alpha" profile */
extern asm_gray_scsp_t bopti_gasm_gray_alpha_scsp;

/* struct rbox: A rendering box (target coordinates and source rectangle)
   Meaning of fields vary during the rendering process! */
struct rbox
{
   /* General renderer:
        On-screen location of the leftmost pixel of the leftmost rendered
        column (this particular pixel might not be drawn but is of
        importance in the positioning process)
      SCSP renderer:
        Shift value used to align columns with positions */
   int x;
   /* On-screen location of top-left corner; the (x,y) of dsubimage() */
   int visual_x, y;
   /* Width of rendered sub-image */
   int width;
   /* Before bopti_render{_scsp}():
        Left-coordinate of the source box (included, in pixels)
      In bopti_render{_scsp}():
        Left-coordinate of the source box (included, in columns) */
   int left;
   /* Number of columns used in the source box */
   int columns;
   /* Vertical bounds of the box in the image (inc-excluded, in pixels) */
   int top, height;
};

//---
// Text rendering
//---

/* Signature of text rendering functions (which do not render text but really
   just blend a column of operators onto the VRAM */
typedef void asm_text_t(uint32_t *v1, uint32_t *v2, uint32_t *op, int height);

/* One rendering function per color */
extern asm_text_t *topti_asm_text[8];

/* topti_render(): Render a string on the VRAM
   Combines glyph data onto VRAM operands and blits them efficiently onto the
   VRAM. To write a single character, use a 2-byte string with a NUL.

   @x @y    Target position on VRAM
   @str     Text source
   @f       Font
   @asm_fg  Assembler function for text rendering
   @asm_bg  Assembler function for background rendering
   @v1      Monochrome VRAM or light gray VRAM
   @v2      Monochrome or dark gray VRAM
   @size    Maximum number of characters to render */
void topti_render(int x, int y, char const *str, font_t const *f,
   asm_text_t *asm_fg, asm_text_t *asm_bg, uint32_t *v1, uint32_t *v2,
   int size);

//---
// Gray rendering functions for dmode
//---

/* These are the corresponding gray rendering functions */
int gupdate(void);
void gclear(color_t color);
void grect(int x1, int y1, int x2, int y2, color_t color);
void gpixel(int x, int y, color_t color);
int ggetpixel(int x, int y);
void gint_ghline(int x1, int x2, int y, int color);
void gint_gvline(int y1, int y2, int x, int color);
void gtext_opt(int x, int y, int fg, int bg, int halign, int valign,
	 char const *str, int size);
void gsubimage(bopti_image_t const *image, struct rbox *r, int flags);

#endif /* GINT_RENDER_MONO */

#endif /* RENDER_FX */
