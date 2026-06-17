//---
//	render - Internal definitions for common display functions
//---

#ifndef RENDER_COMMON
#define RENDER_COMMON

#include <gint/config.h>
#include <gint/display.h>

/* gint_dhline(): Optimized horizontal line
   @x1 @x2 @y  Coordinates of endpoints of line (both included)
   @color      Any color suitable for dline() */
void gint_dhline(int x1, int x2, int y, color_t color);

/* gint_dvline(): Optimized vertical line
   @y1 @y2 @x  Coordinates of endpoints of line (both included)
   @color      Any color suitable for dline() */
void gint_dvline(int y1, int y2, int x, color_t color);

//---
//	Font rendering (topti)
//---

/* Current font */
extern font_t const *topti_font;
/* Default font */
extern font_t const *gint_default_font;

//---
// Alternate rendering modes
//---

#if GINT_RENDER_DMODE

/* The gray engine overrides the rendering functions by specifying a set of
   alternate primitives that are suited to work with two VRAMs. To avoid
   linking with them when the gray engine is not used, the display module
   exposes a global state in the form of a struct rendering_mode and the gray
   engine modifies that state when it runs. */
struct rbox;
struct rendering_mode
{
   /* Because the gray engine still has business to do after the call to
      dgray(DGRAY_OFF), the original dupdate() is made to execute after
      the replacement one if the replacement one returns 1. */
   int (*dupdate)(void);
   /* Area rendering */
   void (*dclear)(color_t color);
   void (*drect)(int x1, int y1, int x2, int y2, color_t color);
   /* Point rendering */
   void (*dpixel)(int x, int y, color_t color);
   int (*dgetpixel)(int x, int y);
   void (*gint_dhline)(int x1, int x2, int y, int color);
   void (*gint_dvline)(int y1, int y2, int x, int color);
   /* Text and image rendering */
   void (*dtext_opt)
      (int x, int y, int fg, int bg, int halign, int valign,
       char const *str, int size);
   void (*dsubimage)
      (bopti_image_t const *image, struct rbox *r, int flags);
};

/* The alternate rendering mode pointer (initially NULL)*/
extern struct rendering_mode const *dmode;

/* Short macro to call the alternate rendering function when available */
#define DMODE_OVERRIDE(func, ...)         \
   if(dmode && dmode->func) {       \
      return dmode->func(__VA_ARGS__); \
   }

#else
#define DMODE_OVERRIDE(func, ...)
#endif /* GINT_RENDER_DMODE */

#endif /* RENDER_COMMON */
