//---
//	gint:gray:engine - Core gray engine
//---

#include <gint/defs/types.h>
#include <gint/drivers/t6k11.h>
#include <gint/gray.h>
#include <gint/display.h>
#include <gint/timer.h>

#include <stdlib.h>

#include "../render-fx/render-fx.h"
#include "../render/render.h"
#include <gint/config.h>

#if GINT_HW_CG
#include <gint/drivers/r61524.h>
#endif

// TODO: Move the gray "engine" part into the T6K11 driver.
#if GINT_RENDER_MONO

/* Three additional video RAMS, allocated statically if --static-gray was set
   at configure time, or with malloc() otherwise. */
#ifdef GINT_STATIC_GRAY
GBSS static uint32_t gvrams[3][256];
#endif

/* Four VRAMs: two to draw and two to display */
static uint32_t *vrams[4] = { NULL, NULL, NULL, NULL };

#if GINT_HW_FX

/* Current VRAM pair used for drawing; the value can either be 0 (draws to
   VRAMs 0 and 1) or 2 (draws to VRAMs 2 and 3). */
static int volatile st = 0;
/* Timer ID, always equal to GRAY_TIMER except if initialization fails */
static int timer = -1;
/* Whether the engine is scheduled to run at the next frame */
static int runs = 0;

/* Underlying timer, set to count at P_phi/64 */
#define GRAY_TIMER 0
#define GRAY_CLOCK TIMER_Pphi_64
/* Delays of the light and dark frames for the above setting */
GBSS static int delays[2];

static int gray_int(void);

#endif

/* The alternate rendering mode structure used to override d*() */
static struct rendering_mode const gray_mode = {
	.dupdate      = gupdate,
	.dclear       = gclear,
	.drect        = grect,
	.dpixel       = gpixel,
	.dgetpixel    = ggetpixel,
	.gint_dhline  = gint_ghline,
	.gint_dvline  = gint_gvline,
	.dtext_opt    = gtext_opt,
	.dsubimage    = gsubimage,
};
static struct rendering_mode const gray_exit_mode = {
	.dupdate      = gupdate,
	.dclear       = NULL,
	.drect        = NULL,
	.dpixel       = NULL,
	.dgetpixel    = NULL,
	.gint_dhline  = NULL,
	.gint_dvline  = NULL,
	.dtext_opt    = NULL,
	.dsubimage    = NULL,
};

//---
//	Engine control (init/quit and start/stop)
//---

static void gray_quit(void);

/* gray_isinit(): Check whether the engine is initialized and ready to run */
static int gray_isinit(void)
{
	return (vrams[0] && vrams[1] && vrams[2] && vrams[3]
#if GINT_HW_FX
		&& timer >= 0
#endif
		);
}

/* gray_init(): Initialize the engine
   This is done at startup so that memory can be reserved very early from the
   heap (because not having enough memory is unrecoverable for the engine). */
GCONSTRUCTOR static void gray_init(void)
{
	/* We need four VRAMs. First use the standard monochrome one */
	vrams[0] = gint_vram;

	#ifdef GINT_STATIC_GRAY
	vrams[1] = gvrams[0];
	vrams[2] = gvrams[1];
	vrams[3] = gvrams[2];
	#else
	vrams[1] = malloc(1024);
	vrams[2] = malloc(1024);
	vrams[3] = malloc(1024);
	#endif /* GINT_STATIC_GRAY */

#if GINT_HW_FX
	/* Default delays from Graph 35+E II are different from other models */
	if(gint[HWCALC] == HWCALC_G35PE2)
	{
		delays[0] = 762;
		delays[1] = 1311;
	}
	else
	{
		delays[0] = 923;
		delays[1] = 1742;
	}

	/* Try to obtain the timer right away */
	timer = timer_configure(GRAY_TIMER | GRAY_CLOCK, 1000,
		GINT_CALL(gray_int));
#endif

	/* On failure, release the resources that we obtained */
	if(!gray_isinit()) gray_quit();
}

/* gray_quit(): Free engine resources */
GDESTRUCTOR static void gray_quit(void)
{
	#ifndef GINT_STATIC_GRAY
	if(vrams[1]) free(vrams[1]);
	if(vrams[2]) free(vrams[2]);
	if(vrams[3]) free(vrams[3]);

	vrams[1] = NULL;
	vrams[2] = NULL;
	vrams[3] = NULL;
	#endif /* GINT_STATIC_GRAY */

#if GINT_HW_FX
	if(timer >= 0) timer_stop(timer);
	timer = -1;
#endif
}

#if GINT_HW_FX
/* gray_start(): Start the gray engine */
static void gray_start(void)
{
	st = 2;
	timer_reload(GRAY_TIMER, delays[0]);
	timer_start(GRAY_TIMER);
	runs = 1;
}

/* gray_stop(): Stop the gray engine */
static void gray_stop(void)
{
	timer_pause(GRAY_TIMER);
	runs = 0;
	st = 0;
}
#endif

//---
//	Dynamic udpate and rendering mode
//---

/* dgray(): Start or stop the gray engine at the next dupdate() */
int dgray(int mode)
{
	/* Stack of states for the push modes */
	static uint8_t states[32] = { 0 };
	static uint8_t current = 0;

	if(mode == DGRAY_ON)
	{
		if(!gray_isinit()) return 1;

		/* Set the display module's alternate rendering mode to
		   override rendering functions to use their g*() variant */
		if(!dgray_enabled()) dmode = &gray_mode;
	}
	else if(mode == DGRAY_OFF)
	{
		/* Set the mode to a temporary one that only overrides
		   dupdate() so that we can stop the engine next frame */
		if(dgray_enabled()) dmode = &gray_exit_mode;
	}
	else if(mode == DGRAY_PUSH_ON)
	{
		if(current >= 32) return 1;
		states[current++] = dgray_enabled() ? DGRAY_ON : DGRAY_OFF;

		return dgray(DGRAY_ON);
	}
	else if(mode == DGRAY_PUSH_OFF)
	{
		if(current >= 32) return 1;
		states[current++] = dgray_enabled() ? DGRAY_ON : DGRAY_OFF;

		return dgray(DGRAY_OFF);
	}
	else if(mode == DGRAY_POP)
	{
		/* Stay at 0 if the user's push/pop logic is broken */
		if(current > 0) current--;

		/* Switch to previous state */
		return dgray(states[current]);
	}
	else return 1;

	return 0;
}

#if GINT_HW_FX
/* gray_int(): Interrupt handler */
int gray_int(void)
{
	t6k11_display(vrams[st ^ 2], 0, 64, 16);
	timer_reload(GRAY_TIMER, delays[(st ^ 3) & 1]);
	st ^= 1;

	return TIMER_CONTINUE;
}

/* gupdate(): Push the current VRAMs to the screen */
int gupdate(void)
{
	/* At the first gupdate(), start the engine */
	if(dmode == &gray_mode && !runs)
	{
		gray_start();
		return 0;
	}
	/* At the last gupdate(), stop the engine */
	if(dmode == &gray_exit_mode)
	{
		gray_stop();
		dmode = NULL;
		return 1;
	}

	/* When the engine is running, swap frames */
	st ^= 2;
	return 0;
}
#elif GINT_HW_CG
int gupdate(void)
{
	if(dmode == &gray_exit_mode)
	{
		dmode = NULL;
		return 1;
	}

	r61524_display_gray_128x64(vrams[0], vrams[1]);
	return 0;
}
#endif

//---
//	Query and configuration functions
//---

/* dgray_enabled(): Check whether gray mode is enabled */
int dgray_enabled(void)
{
	return (dmode == &gray_mode);
}

#if GINT_HW_FX
/* dgray_setdelays(): Set the gray engine delays */
void dgray_setdelays(uint32_t light, uint32_t dark)
{
	delays[0] = light;
	delays[1] = dark;
}

/* dgray_getdelays(): Get the gray engine delays */
void dgray_getdelays(uint32_t *light, uint32_t *dark)
{
	if(light) *light = delays[0];
	if(dark) *dark = delays[1];
}

/* dgray_getvram(): Get the current VRAM pointers */
void dgray_getvram(uint32_t **light, uint32_t **dark)
{
	int base = st;

	if(light) *light = vrams[base & 2];
	if(dark) *dark = vrams[base | 1];
}

/* dgray_getscreen(): Get the current screen pointers */
void dgray_getscreen(uint32_t **light, uint32_t **dark)
{
	int base = st ^ 2;

	if(light) *light = vrams[base & 2];
	if(dark) *dark = vrams[base | 1];
}
#elif GINT_HW_CG
void dgray_getvram(uint32_t **light, uint32_t **dark)
{
	*light = vrams[0];
	*dark = vrams[1];
}
#endif

#endif /* GINT_RENDER_MONO && GINT_HW_FX */
