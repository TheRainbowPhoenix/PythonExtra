//---
//	gint:t6k11 - Toshiba T6K11 driver
//---

#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/drivers/t6k11.h>

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/hardware.h>
#include <gint/config.h>

#if GINT_HW_FX

//---
//	Device specification sheet
//---

/* This version number is 1 for the old T6K11 everyone knows, and 2 for the
   newer one found in the Graph 35+E II. Documentation is available only for
   version 1. Dumps of Bdisp_PutDisp_DD() are used to drive version 2. */
static int t6k11_version = 1;

/* Screen registers on the original T6K11. Registers 8..11 and 13..15 are test
   registers and must not be used! */
enum {
	reg_display	= 0,
	reg_counter	= 1,
	reg_analog	= 2,
	reg_alternate	= 3,

	reg_yaddr	= 4,	/* These two use the same register */
	reg_xaddr	= 4,	/* (interpretation depends on count mode) */

	reg_zaddr	= 5,
	reg_contrast	= 6,
	reg_data	= 7,
	reg_daconv	= 12,
};

/* Automatic counter increment during read/write */
enum {
	cnt_ydown	= 0,
	cnt_yup		= 1,
	cnt_xdown	= 2,
	cnt_xup		= 3,
};

//---
//	Device communication primitives
//---

/* I/O may be performed either with RS = 0 or RS = 1. The memory-mapping of the
   device I/O maps bit 16 of the address to pin RS. There may be other mapped
   pins in the address. (?) */

/* RS = 0: Register selection */
static volatile uint8_t *sel = (void *)0xb4000000;
/* RS = 1: Command data or vram data */
static volatile uint8_t *cmd = (void *)0xb4010000;

/* command() - send a command to set the value of a register
   @reg   Register number
   @data  Value to set in reg */
GINLINE static void command(uint8_t reg, uint8_t data)
{
	*sel = reg;
	*cmd = data;
}

/* status() - read the status byte from the display driver */
GINLINE static uint8_t status(void)
{
	return *sel;
}

/* write_row() - send 16 bytes to the display driver
   @buf  Buffer to take data from */
GINLINE static void write_row(const uint8_t *buf)
{
	/* Unroll the loop for more speed */
	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;

	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;

	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;

	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;
	*cmd = *buf++;
}

//---
//	Driver functions
//---

/* t6k11_display() - send vram data to the LCD device */
void t6k11_display_v1(const void *vram, int y1, int y2, size_t stride)
{
	for(int y = y1; y < y2; y++)
	{
		/* Set the X-address register for this row */
		command(reg_xaddr, y | 0xc0);
		/* Use Y-Up mode */
		command(reg_counter, cnt_yup);
		/* Start counting Y from 0 */
		command(reg_yaddr, 0);

		/* Send the row's data to the device */
		*sel = reg_data;
		write_row(vram);
		vram += stride;
	}
}
void t6k11_display_v2(const void *vram, int y1, int y2, size_t stride)
{
	for(int y = y1; y < y2; y++)
	{
		command(8, y | 0x80);
		command(8, 4);

		*sel = 10;
		write_row(vram);
		vram += stride;
	}
}
void t6k11_display(const void *vram, int y1, int y2, size_t stride)
{
	if(t6k11_version == 1) t6k11_display_v1(vram, y1, y2, stride);
	if(t6k11_version == 2) t6k11_display_v2(vram, y1, y2, stride);
}

/* t6k11_contrast() - change the contrast setting */
void t6k11_contrast(int contrast)
{
	if(contrast < 0)  contrast = 0;
	if(contrast > 32) contrast = 32;

	/* Reasonable values for contrast, taken from the diagnostic mode of an
	   fx9860G II with OS 02.05.2201, are in range 0x97 .. 0xb7.
	   This means VLC0 = 2 and CONTRAST in [23..55] */
	command(reg_contrast, 0x97 + contrast);

	/* TODO: Turns out that different models, notably screens without
	   TODO: backlight, will have different ranges. Plus we might want to
	   TODO: use transitions to extreme contrast settings for visual
	   TODO: effects.
	   TODO: Extend the available range of contrast settings. */
}

/* t6k11_backlight() - manage the screen backlight */
void t6k11_backlight(int setting)
{
	volatile uint8_t *port;
	uint8_t mask;

	/* This setting is mapped to an I/O port:
	   - On SH3, bit 7 of port G
	   - On SH4, bit 4 of port N
	   TODO: Document the PFC to remove these addresses */
	if(isSH3())
	{
		if(isSlim())
		{
			port = (void *)0xa4000126;
			mask = 0x20;
		}
		else
		{
			port = (void *)0xa400012c;
			mask = 0x80;
		}
	}
	else
	{
		port = (void *)0xa4050138;
		mask = 0x10;
	}

	if(!setting) *port &= ~mask;
	if(setting > 0) *port |= mask;
	if(setting < 0) *port ^= mask;
}

static void constructor(void)
{
	if(gint[HWCALC] == HWCALC_G35PE2) t6k11_version = 2;
}

//---
// State and driver metadata
//---

static void hsave(t6k11_state_t *s)
{
	if(t6k11_version == 2) return;
	s->STRD = status();
}

static void hrestore(t6k11_state_t const *s)
{
	if(t6k11_version == 2) return;

	/* Set an X-address of 0 with the original display mode */
	uint8_t nf = (s->STRD & 0x04) >> 2;
	command(reg_xaddr, 0x80 | (nf << 6));

	/* Restore the counter mode */
	uint8_t cnt = (s->STRD & 0x03);
	command(reg_counter, cnt);
}

gint_driver_t drv_t6k11 = {
	.name         = "T6K11",
	.constructor  = constructor,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(t6k11_state_t),
};
GINT_DECLARE_DRIVER(26, drv_t6k11);

#endif /* GINT_HW_FX */
