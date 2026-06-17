//---
//	gint:keysc:iokbd - I/O-based keyboard input
//---

#include <gint/defs/types.h>
#include <gint/mpu/pfc.h>
#include <gint/hardware.h>
#include <gint/config.h>

/* This file is SH7705-only. */
#if GINT_HW_FX
#define PFC SH7705_PFC

/* iokbd_delay() - wait a bit so that I/O can keep up
   May use the watchdog timer, but the keyboard driver will need to save it. */
static void iokbd_delay(void)
{
	__asm__(
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
	);

#if 0
	uint8_t  volatile *WTCSRr = (void *)0xffffff86;
	uint16_t volatile *WTCSRw = (void *)0xffffff86;
	uint16_t volatile *WTCNTw = (void *)0xffffff84;

	/* Watchdog delay version, each of the values between this and 0xff
	   account for 256 clock cycles on Pphi. This is roughly equivalent to
	   2048 nop, lasting ~70 µs */
	const int delay = 0xf4;

	/* Disable the watchdog timer interrupt and reset configuration */
	*WTCSRw = 0xa500;

	/* Set the delay, input on Pphi / 256 and start counting */
	*WTCNTw = 0x5a00 | (delay & 0xff);
	*WTCSRw = 0xa505;
	*WTCSRw = 0xa585;

	/* Actively wait for overflow, then clear the interrupt flag */
	while((*WTCSRr & 0x08) == 0);
	*WTCSRw = 0xa500 | (*WTCSRr & 0xf7);

	/* Reset configuration, counter, and re-enabled interrupt */
	*WTCSRw = 0xa500;
	*WTCNTw = 0x5a00;
#endif
}

/* iokbd_row() - acquire hit status of a single row from the I/O ports
   @row  Requested row number, 0..9
   Returns 8 bits of key state. */
uint8_t iokbd_row(int row)
{
	if((unsigned)row > 9) return 0x00;

	int orig_PBCR = PFC.PBCR;
	int orig_PMCR = PFC.PMCR;
	int orig_PBDR = PFC.PBDR;
	int orig_PMDR = PFC.PMDR;

	/* This will enable output (01) on @row, input (10) everywhere else */
	uint16_t ctrl_mask = 0x0003 << ((row & 7) * 2);
	/* Enable output (0) on @row, input (1) everywhere else */
	uint8_t data_mask = ~(1 << (row & 7));

	/* When row < 8, the associated bits are in port B */
	if(row < 8)
	{
		/* Set @row as output in port B; port M is unused */
		PFC.PBCR = 0xaaaa ^ ctrl_mask;
		PFC.PMCR = (PFC.PMCR & 0xff00) | 0x00aa;
		iokbd_delay();

		/* Set @row to 0, everything else to 1 */
		PFC.PBDR = data_mask;
		PFC.PMDR = (PFC.PMDR & 0xf0) | 0x0f;
		iokbd_delay();
	}
	/* When row >= 8, the associated bits are in port M */
	else
	{
		/* Set @row as output in port M; port B is unused */
		PFC.PBCR = 0xaaaa;
		PFC.PMCR = (PFC.PMCR & 0xff00) | (0x00aa ^ ctrl_mask);
		iokbd_delay();

		/* Set @row to 0, everything else to 1 */
		PFC.PBDR = 0xff;
		PFC.PMDR = PFC.PMDR & data_mask;
		iokbd_delay();
	}

	/* Now read the input data from the keyboard! */
	uint8_t input = ~PFC.PADR;
	iokbd_delay();

	/* Reset the port configuration */
	PFC.PBCR = orig_PBCR;
	PFC.PMCR = orig_PMCR;
	iokbd_delay();

	/* Now also reset the data registers. This was forgotten from SimLo's
	   CheckKeyRow() and blows up everything. */
	PFC.PBDR = orig_PBDR;
	PFC.PMDR = orig_PMDR;
	iokbd_delay();

	return input;
}

static const uint16_t SLIM_SC[] = {
	0x0940, 0x0920, 0x0910, 0x0908,
	0x0840, 0x0820, 0x0810, 0x0808, 0x0804, 0x0802,
	0x0740, 0x0720, 0x0710, 0x0708, 0x0704, 0x0702,
	0x0640, 0x0620, 0x0610, 0x0608, 0x0604, 0x0602,
	0x0540, 0x0520, 0x0510, 0x0508, 0x0504, 0x0502,
	0x0440, 0x0420, 0x0410, 0x0408, 0x0404, 0x0402,
	0x0340, 0x0320, 0x0310, 0x0308, 0x0304, 0x0302,
	0x0240, 0x0220, 0x0210, 0x0208, 0x0204, 0x0202,
	        0x0120, 0x0110, 0x0108, 0x0104, 0x0102,
	                                                0x0001
};

#define SCANCODE_COUNT (sizeof(SLIM_SC) / sizeof(uint16_t))

static const uint16_t SLIM_TR[] = {
	0x0808, 0x0640, 0x0840, 0x0740,
	0x0940, 0x0620, 0x0720, 0x0710, 0x0804, 0x0802,
	0x0920, 0x0610, 0x0504, 0x0820, 0x0704, 0x0702,
	0x0910, 0x0608, 0x0502, 0x0810, 0x0280, 0x0180,
	0x0908, 0x0604, 0x0440, 0x0340, 0x0240, 0x0140,
	0x0904, 0x0602, 0x0420, 0x0320, 0x0220, 0x0120,
	0x0902, 0x0540, 0x0410, 0x0310, 0x0210, 0x0110,
	0x0708, 0x0520, 0x0408, 0x0308, 0x0208, 0x0108,
	        0x0510, 0x0508, 0x0304, 0x0104, 0x0204,
	                                                0x0001
};

/* iokbd_scan() - scan ports A/B/M to generate 12 rows of key data */
void iokbd_scan(uint8_t *scan)
{
	/* Scan each row independently; the gain from scanning them all together
	   is probably not worth it */
	for(int i = 0; i < 12; i++) scan[i] = iokbd_row(i);

	/* Translate fx-9860G Slim scancodes to standard scancodes */
	if(isSlim())
	{
		uint8_t slim_scan[12];
		for(uint i = 0; i < 10; i++)
		{
			slim_scan[i] = scan[i];
			scan[i] = 0x00;
		}

		for(uint i = 0; i < SCANCODE_COUNT; i++)
			if(slim_scan[SLIM_SC[i] >> 8] & (SLIM_SC[i] & 0xFF))
				scan[SLIM_TR[i] >> 8] |= (SLIM_TR[i] & 0xFF);
	}
}

#endif /* GINT_HW_FX */
