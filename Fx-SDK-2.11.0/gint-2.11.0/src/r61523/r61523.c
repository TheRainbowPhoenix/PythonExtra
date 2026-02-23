// Renesas R61523 driver-

#include <gint/defs/types.h>
#include <gint/defs/util.h>
// #include <gint/hardware.h>
#include <gint/drivers.h>
// #include <gint/dma.h>
#include <gint/drivers/r61523.h>
#include <gint/video.h>
#include <gint/image.h>
#include <gint/config.h>

#if GINT_HW_CP

/* Registers */
#define REG_HRANGE                0x2a
#define REG_VRANGE                0x2b
#define REG_DATA                  0x2c
#define REG_DEVICE_CODE_READ      0xbf

/* Interface with the controller */
static volatile uint16_t *DISPLAY = (void *)0xb4000000;
/* Bit 4 of Port R controls the RS bit of the display driver */
static volatile uint8_t *PRDR = (void *)0xa405013c;

/* Select a register */
GINLINE static void select(uint16_t reg)
{
    /* Clear RS and write the register number */
    *PRDR &= ~0x10;
    synco();
    *DISPLAY = reg;
    synco();
    /* Set RS=1 to allow consecutive reads/writes after a select() */
    *PRDR |= 0x10;
    synco();
}

GINLINE static void write(uint16_t data)
{
    *DISPLAY = data;
}

GINLINE static uint16_t read(void)
{
    return *DISPLAY;
}

static void read_Nu16(uint16_t *array, int N)
{
    for(int i = 0; i < N; i++)
        array[i] = *DISPLAY;
}

//---
// Generic functions
//---

void r61523_identify(uint32_t *manufacturerCode, uint16_t *deviceCode)
{
    select(REG_DEVICE_CODE_READ);
    uint16_t packets[5];
    read_Nu16(packets, 5);

    if(manufacturerCode)
        *manufacturerCode = (packets[1] << 16) | packets[2];
    if(deviceCode)
        *deviceCode = (packets[3] << 16) | packets[4];
}

//---
// Window management
//---

void r61523_win_set(int x1, int x2, int y1, int y2)
{
    /* R61523 has a 360x640 area; the CP-400 uses the top-right corner for its
       320x528 display, so skip over the first 40 columns */
    x1 += 40;
    x2 += 40;

    uint16_t volatile *DISPLAY = (void *)0xb4000000;

    select(REG_HRANGE);

    /* Upper half has 2 bits (total 10 bits = 1024) */
    *DISPLAY = (x1 >> 8) & 3;
    synco();
    *DISPLAY = x1 & 0xff;

    *DISPLAY = (x2 >> 8) & 3;
    synco();
    *DISPLAY = x2 & 0xff;
    synco();

    select(REG_VRANGE);

    *DISPLAY = (y1 >> 8) & 3;
    synco();
    *DISPLAY = y1 & 0xff;
    synco();

    *DISPLAY = (y2 >> 8) & 3;
    synco();
    *DISPLAY = y2 & 0xff;
    synco();
}

void r61523_display(uint16_t *vram)
{
    r61523_win_set(0, 319, 0, 527);

    select(44);

    int row_offset = 0;
    uint16_t volatile *DISPLAY = (void *)0xb4000000;

    for(int y = 0; y < 528; y++) {
        uint16_t *r5 = (void *)vram + row_offset;

        for(int x = 0; x < 320; x++)
            *DISPLAY = *r5++;

        row_offset += 2 * 320;
    }
}

static bool r61523_update(int x, int y, image_t const *fb, int flags)
{
    if(fb->format != IMAGE_RGB565)
        return false;

    // TODO: r61523_update: DMA support
    // unless VIDEO_UPDATE_FOREIGN_WORLD is set
    (void)flags;
    uint w = fb->width;
    uint h = fb->height;

    // TODO: r61523_update: sub-rectangle support
    if(x != 0 || y != 0 || w != 320 || h != 528)
        return false;

    // TODO: r61523_update: stride support!
    if(fb->stride != 320 * 2)
        return false;

    r61523_display(fb->data);
    return true;
}

//---
// Driver metadata
//---

/* As far as I can tell there's no way to read the current window from the
   controller so this driver is completely stateless for now. */
gint_driver_t drv_r61523 = {
    .name         = "R61523",
};
GINT_DECLARE_DRIVER(26, drv_r61523);

//---
// Video driver interface
//---

static video_mode_t r61523_modes[] = {
    /* Standard full-screen full-color mode */
    { 320, 528, IMAGE_RGB565, -1 },
    { 0 }
};

video_interface_t r61523_video = {
    .driver          = &drv_r61523,
    .modes           = r61523_modes,
    .mode_get        = NULL, // TODO
    .mode_set        = NULL, // TODO
    .brightness_min  = 0, // TODO
    .brightness_max  = 0, // TODO
    .brightness_set  = NULL,
    .update          = r61523_update,
};

#endif /* GINT_HW_CP */
