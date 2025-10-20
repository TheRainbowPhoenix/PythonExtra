//---
//	gint:r61524 - Renesas R61524 driver
//---

#include <gint/defs/types.h>
#include <gint/defs/util.h>
#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/dma.h>
#include <gint/drivers/r61524.h>
#include <gint/video.h>
#include <gint/image.h>
#include <gint/config.h>

#if GINT_HW_CG

/* Registers */
#define REG_DEVICE_CODE_READ    0x000
#define REG_DRIVER_OUTPUT_CTL   0x001
#define REG_ENTRY_MODE          0x003
#define REG_DISPLAY_CTL2        0x008
#define REG_LOW_POWER_CTL       0x00b
#define REG_HADDR               0x200
#define REG_VADDR               0x201
#define REG_DATA                0x202
#define REG_HSTART              0x210
#define REG_HEND                0x211
#define REG_VSTART              0x212
#define REG_VEND                0x213

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

GINLINE uint16_t r61524_get(int ID)
{
	select(ID);
	return *DISPLAY;
}

GINLINE void r61524_set(int ID, uint16_t value)
{
	select(ID);
	*DISPLAY = value;
}

//---
// Window management
//---

void r61524_win_get(uint16_t *HSA, uint16_t *HEA, uint16_t *VSA, uint16_t *VEA)
{
	*HSA = r61524_get(REG_HSTART);
	*HEA = r61524_get(REG_HEND);
	*VSA = r61524_get(REG_VSTART);
	*VEA = r61524_get(REG_VEND);
}

void r61524_win_set(uint16_t HSA, uint16_t HEA, uint16_t VSA, uint16_t VEA)
{
	r61524_set(REG_HSTART, HSA);
	r61524_set(REG_HEND, HEA);
	r61524_set(REG_VSTART, VSA);
	r61524_set(REG_VEND, VEA);
}

//---
// Backlight management
//---

void r61525_brightness_set(int level)
{
	bool GLOBAL_backlight_high_bit[7] = { /* at 0x80399530 */
	    false, false, false, false,
	    true, true, true,
	};
	uint8_t GLOBAL_backlight_table[7] = { /* at 0x80399537 */
	    0x14, 0x4b, 0x78, 0xd2,
	    0x6e, 0xa0, 0xc8,
	};
	uint8_t GLOBAL_0[10] = { /* at 0x8039953e */
	    0x00, 0x01, 0x02, 0x03, 0x07,
	    0x0f, 0x1f, 0x3f, 0x7f, 0xff,
	};

	if(level < 1)
		level = 1;
	if(level > 5)
		level = 5;

	int8_t volatile *PNDR = (void *)0xa4050138;
	if(GLOBAL_backlight_high_bit[level])
		*PNDR |= 0x10;
	else
		*PNDR &= 0xef;

	synco();

	r61524_set(0x5a2, GLOBAL_0[(level < 2) ? 9 : 5]);
	r61524_set(0x5a1, GLOBAL_backlight_table[level]);
}

//---
//	Driver functions
//---

/* TODO: r61524: update, backlight, brightness, gamma */

void r61524_start_frame(int xmin, int xmax, int ymin, int ymax)
{
	/* Move the window to the desired region, then select address 0 */
	r61524_win_set(395-xmax, 395-xmin, ymin, ymax);
	r61524_set(REG_HADDR, 0);
	r61524_set(REG_VADDR, 0);

	/* Bind address 0xb4000000 to the data write command */
	select(REG_DATA);
}

void r61524_display(uint16_t *vram, int start, int height, int method)
{
	/* Wait for any transfer to finish before using the screen, otherwise
	   the DMA might write data *while* we're sending commands! */
	dma_transfer_wait(0);

	r61524_start_frame(0, 395, start, start + height - 1);

	if(method == R61524_CPU)
	{
		for(int i = 0; i < 396 * height; i++)
			write(vram[i + 396 * start]);
		return;
	}

	void *src = (void *)vram + start * 396*2;
	void *dst = (void *)0xb4000000;

	/* The amount of data sent per row, 396*2, is not a multiple of 32. For
	   now I assume [height] is a multiple of 4, which makes the factor 32
	   appear. */
	int blocks = 99 * (height >> 2);

	if(method == R61524_DMA) {
		dma_transfer_async(0, DMA_32B, blocks, src, DMA_INC, dst,
			DMA_FIXED, GINT_CALL_NULL);
	}
	else {
		/* Transfer atomically */
		dma_transfer_atomic(0, DMA_32B, blocks, src, DMA_INC, dst,
			DMA_FIXED);
	}
}

void r61524_display_rect(uint16_t *vram, int xmin, int xmax, int ymin,
	int ymax)
{
	dma_transfer_wait(0);
	r61524_start_frame(xmin, xmax, ymin, ymax);

	vram += 396 * ymin + xmin;

	for(int y = 0; y < ymax - ymin + 1; y++) {
		for(int x = 0; x < xmax - xmin + 1; x++)
			write(vram[x]);
		vram += 396;
	}
}

void r61524_display_mono_128x64(uint32_t *vram)
{
	dma_transfer_wait(0);
	r61524_start_frame(0, 395, 0, 223);

	int border = 0xe71c; /* C_RGB(28, 28, 28) */

	for(int i = 0; i < 16 * 396; i++)
		write(border);

	for(int y = 0; y < 64; y++) {
		/* sub-y position */
		for(int sy = 0; sy < 3; sy++) {
			for(int i = 0; i < 6; i++)
				write(border);

			/* longword-x position */
			for(int lwx = 0; lwx < 4; lwx++) {
				int32_t i = vram[lwx];
				/* sub-x position */
				for(int sx = 0; sx < 32; sx++) {
					int color = ~(i >> 31);
					i <<= 1;
					write(color);
					write(color);
					write(color);
				}
			}

			for(int i = 0; i < 6; i++)
				write(border);
		}
		vram += 4;
	}

	for(int i = 0; i < 16 * 396; i++)
		write(border);
}

void r61524_display_gray_128x64(uint32_t *light, uint32_t *dark)
{
	dma_transfer_wait(0);
	r61524_start_frame(0, 395, 0, 223);

	int border = 0xe71c; /* C_RGB(28, 28, 28) */
	int colors[] = { 0x0000, 0x528a, 0xad55, 0xffff };

	for(int i = 0; i < 16 * 396; i++)
		write(border);

	for(int y = 0; y < 64; y++) {
		/* sub-y position */
		for(int sy = 0; sy < 3; sy++) {
			for(int i = 0; i < 6; i++)
				write(border);

			/* longword-x position */
			for(int lwx = 0; lwx < 4; lwx++) {
				int32_t il = light[lwx];
				int32_t id = dark[lwx];
				/* sub-x position */
				for(int sx = 0; sx < 32; sx++) {
					int color = colors[(id >= 0) * 2 + (il >= 0)];
					il <<= 1;
					id <<= 1;
					write(color);
					write(color);
					write(color);
				}
			}

			for(int i = 0; i < 6; i++)
				write(border);
		}
		light += 4;
		dark += 4;
	}

	for(int i = 0; i < 16 * 396; i++)
		write(border);
}

static bool r61524_update(int x, int y, image_t const *fb, int flags)
{
	// TODO: r61524_update: Handle the mono cases
	if(fb->format != IMAGE_RGB565)
		return false;

	uint w = fb->width;
	uint h = fb->height;

	dma_transfer_wait(0);
	r61524_start_frame(x, x+w-1, y, y+h-1);

	/* DMA if enabled */
	bool dma_possible = (!x && w == 396 && fb->stride == 396*2 && !(h%4));
	if((flags & VIDEO_UPDATE_ENABLE_DMA) && dma_possible) {
		void *src = fb->data;
		void *dst = (void *)DISPLAY;
		int blocks = 99 * (h / 4);

		if(flags & VIDEO_UPDATE_ATOMIC)
			dma_transfer_atomic(0, DMA_32B, blocks, src, DMA_INC,
				dst, DMA_FIXED);
		else
			dma_transfer_async(0, DMA_32B, blocks, src, DMA_INC,
				dst, DMA_FIXED, GINT_CALL_NULL);
		return true;
	}

	uint16_t *pixels = fb->data;

	for(int y = 0; y < fb->height; y++) {
		for(int x = 0; x < fb->width; x++)
			write(pixels[x]);
		pixels = (void *)pixels + fb->stride;
	}

	return true;
}

//---
// State and driver metadata
//---

static void hsave(r61524_state_t *s)
{
	r61524_win_get(&s->HSA, &s->HEA, &s->VSA, &s->VEA);
}

static void hrestore(r61524_state_t const *s)
{
	r61524_win_set(s->HSA, s->HEA, s->VSA, s->VEA);
}

gint_driver_t drv_r61524 = {
	.name         = "R61524",
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(r61524_state_t),
};
GINT_DECLARE_DRIVER(26, drv_r61524);

//---
// Video driver interface
//---

static video_mode_t r61524_modes[] = {
	/* Standard full-screen full-color mode */
	{ 396, 224, IMAGE_RGB565, -1 },
#if 0
	/* R61524 8-color mode with lower power consumption */
	{ 396, 224, IMAGE_P8_RGB565, -1 }, // TODO: actually P3, that's closest
	/* T6K11-emulation black-and-white mode */
	{ 128, 64, IMAGE_I1MSB, -1 },
	/* T6K11-emulation gray mode */
	{ 128, 64, IMAGE_2I1MSB, -1 },
#endif
	{ 0 }
};

video_interface_t r61524_video = {
	.driver          = &drv_r61524,
	.modes           = r61524_modes,
	.mode_get        = NULL, // TODO
	.mode_set        = NULL, // TODO
	.brightness_min  = 0, // TODO
	.brightness_max  = 0, // TODO
	.brightness_set  = NULL,
	.update          = r61524_update,
};

#endif /* GINT_HW_CG */
