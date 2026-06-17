//---
// gint:video - Generic video interface
//
// This header defines the interface for video (display) drivers. It allows
// high-level code to manipulate the display independently of the underlying
// hardware.
//---

#ifndef GINT_VIDEO
#define GINT_VIDEO

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/drivers.h>
#include <gint/image.h>

/* Video mode offered by a driver for rendering. */
typedef struct {
    /* Mode size */
    uint16_t width;
    uint16_t height;
    /* Pixel format */
    int16_t format;
    /* Refresh frequency, -1 if unknown */
    int16_t freq;
} video_mode_t;

/* Flags for the update function of the interface. */
#define VIDEO_UPDATE_NONE           0x00
#define VIDEO_UPDATE_ENABLE_DMA     0x01
#define VIDEO_UPDATE_ATOMIC         0x02
#define VIDEO_UPDATE_FOREIGN_WORLD  0x04

/* Video driver interface. */
typedef struct {
    /* Associated driver (NULL if there's none). */
    gint_driver_t const *driver;

    /* List of modes (terminated by an all-0 element). The first mode is the
       default mode and it should always be available. */
    video_mode_t const *modes;
    /* Get current video mode. */
    uint (*mode_get)(void);
    /* Set a video mode. */
    bool (*mode_set)(uint id);

    /* Minimum and maximum brightness settings. */
    int brightness_min;
    int brightness_max;
    /* Set a brightness setting. */
    bool (*brightness_set)(int setting);

    /* Implements video_update(); bounds are checked befoer calling. */
    bool (*update)(int x, int y, image_t const *framebuffer, int flags);

} video_interface_t;

/* Get the video interface currently in use. This can be NULL if the program is
   being linked without a display driver. */
video_interface_t const *video_get_current_interface(void);

/* Get the index of the current video mode. This can be -1 if there is no
   interface; however, if there is an interface, this is always >= 0. */
int video_get_current_mode_index(void);

/* Get the a pointer to the current video mode's definition. */
video_mode_t const *video_get_current_mode(void);

/* Update the contents of the display from a framebuffer image. A combination
   of `VIDEO_UPDATE_*` flags can be specified to select the update method:
   - `ENABLE_DMA` allows the driver to copy using DMA, when applicable;
   - `ATOMIC` requires the driver to use an interrupt-less method.
   Returns true on success.

   Update flags will be ignored if not applicable (e.g. `ENABLE_DMA` for a
   video interface that doesn't support DMA) but will result in an error if
   applicable and the specified method fails (e.g. DMA transfer error).

   This function is usually called with (x,y) = (0,0) and a contiguous
   framebuffer whose size is the video mode size. Specifying images with other
   sizes, positions and strides is allowed only when they result in data
   transfers that are byte-aligned. DMA support is only guaranteed for
   contiguous input and output. The implied rectangle must be in-bounds. */
bool video_update(int x, int y, image_t const *fb, int flags);

#ifdef __cplusplus
}
#endif

#endif /* GINT_VIDEO */
