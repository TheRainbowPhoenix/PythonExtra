//---
//	gint:gray - Gray engine control
//---

#ifndef GINT_GRAY
#define GINT_GRAY

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/display.h>

/* Commands for the gray engine */
enum {
	/* Start or stop the engine */
	DGRAY_ON,
	DGRAY_OFF,
	/* Start or stop the engine, but remember previous state */
	DGRAY_PUSH_ON,
	DGRAY_PUSH_OFF,
	/* Restore previous state remembered by DGRAY_PUSH_* */
	DGRAY_POP,
};

/* dgray(): Start or stop the gray engine at the next dupdate()

   This function configures the display module to work with or without the gray
   engine. When the mode is set to DGRAY_ON, all rendering functions are
   replaced to use gray rendering, and gray frames will start being displayed
   after the next dupdate(). To transition from monochrome rendering to gray
   rendering, you must first call dgray(DGRAY_ON), then draw your first gray
   frame from scratch, then call dupdate().

   Similarly, when the mode is DGRAY_OFF, all rendering functions are replaced
   to use monochrome rendering, and the next frame sent with dupdate() will
   stop the gray engine on the display. To transition back to monochrome
   rendering, call dgray(DGRAY_OFF), draw your first monochrome frame from
   scratch, then call dupdate().

   The gray engine uses the timer id GRAY_TIMER (which is 0) to run and needs 3
   additional VRAMs (totaling 4), obtained on the heap by default. Timer 0 is
   chosen for its precision and high priority. If GRAY_TIMER is unavailable or
   VRAMs cannot be obtained with malloc(), the gray engine will fail to start
   and this function will return a non-zero error code.

   When functions that use the gray engine call functions that don't and vice-
   versa, it can be tedious to know exacly when the gray engine should be
   running. To solve this problem, this function provides DGRAY_PUSH_ON and
   DGRAY_PUSH_OFF that save the current state of the engine before starting or
   stopping it. DGRAY_POP can later be used to transition back to the saved
   state. 32 states can be saved, meaning that up to 32 DGRAY_PUSH_* can be in
   effect simultaneously. This allows each function to set its own state
   without altering the state of their callers.

   This function returns non-zero if the engine has failed to initialize at
   startup (because it failed to obtain either GRAY_TIMER or some of its VRAM
   memory). Because changes to the gray engine only apply after a call to
   dupdate(), you can check if the engine is successfully initialized with:

     int engine_successfully_initialized = dgray(DGRAY_ON);
     dgray(DGRAY_OFF);

   @mode  DGRAY_ON or DGRAY_PUSH_ON to turn the engine on;
          DGRAY_OFF or DGRAY_PUSH_OFF to turn the engine off;
          DGRAY_POP to restore the state as before the last DGRAY_PUSH_*.

   Returns 0 on success and non-zero if the engine has failed to initialize. */
int dgray(int mode);

/* dgray_enabled(): Check whether gray mode is enabled
   Returns non-zero if gray mode is enabled, that is if the gray engine is
   planning to run at the next call to dupdate(). (This is true between the
   calls to dgray(DGRAY_ON) and dgray(DGRAY_OFF).) */
int dgray_enabled(void);

/* dgray_setdelays(): Set the gray engine delays

   The gray engine works by swapping two images at a fast pace. Pixels that are
   white on both or black or both will appear as such, but pixels that are
   black on only one of the images will look gray.

   If both images stay on-screen for the same amount on time, there will be
   only one shade of gray. But if one stays longer, then pixels that are only
   black here will look darker than their counterparts, making two shades of
   gray. This is the default.

   Since the gray engine has its default settings, you don't need to set the
   delays before using gray drawing functions. But you can do it to customize
   the appearance of the gray to your application. Be aware that most values
   will just produce visual artifacts and will not look good. There are three
   characteristics that you'll want to control:

   * The stability of the shades; depending on the frequency gray areas can
     appear to blink.
   * Stripes. If the refresh timing coincides with the screen's display
     duration, stripes will appear. They can be of various sizes, visibility
     and speed; but overall they're the *one* thing you want to avoid.
   * And the color of the shades themselves.

   Here are values from the old fx-9860G-like family, which is every monochrome
   calculator but the Graph 35+E II:

     LIGHT    DARK    BLINKING     STRIPES       COLORS
     --------------------------------------------------
       898    1350        none      common         good
      1075    1444         low      common    too close
       609     884      medium    terrible     variable
       937    1333         low      common       decent
       923    1742        none    scanline         good  [default]
     --------------------------------------------------

  Here are values for the Graph 35+E II only:

     LIGHT    DARK    BLINKING     STRIPES       COLORS
     --------------------------------------------------
       680    1078         low      common         good
       762    1311         low        some         good  [default]
       869    1097      medium        some    too close
       869    1311      medium        none         good
       937    1425        none         bad         good
     --------------------------------------------------

   This function sets the delays of the gray engine. It is safe to call while
   the engine is running, although for best visual effects it is better to call
   it prior to dgray(GRAY_ON).

   @light  New light delay
   @dark   New dark delay */
void dgray_setdelays(uint32_t light, uint32_t dark);

/* dgray_getdelays(): Get the gray engine delays

   Provides the value of the current light and dark delays, measured in timer
   ticks of prescaler P_phi/64. See <gint/clock.h> on how to obtain this value.
   Both pointers may be NULL.

   @light  Set to the current light delay setting
   @dark   Set to the current dark delay setting */
void dgray_getdelays(uint32_t *light, uint32_t *dark);

//---
//	VRAM management
//---

/* dgray_getvram(): Get the current VRAM pointers
   These pointers can be used for custom rendering functions. */
void dgray_getvram(uint32_t **light, uint32_t **dark);

/* dgray_getscreen(): Get the current screen pointers
   These pointers can be used to make screen captures. */
void dgray_getscreen(uint32_t **light, uint32_t **dark);

#ifdef __cplusplus
}
#endif

#endif /* GINT_GRAY */
