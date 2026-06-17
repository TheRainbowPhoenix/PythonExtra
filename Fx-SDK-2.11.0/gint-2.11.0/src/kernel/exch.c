#include <gint/exc.h>
#include <gint/display.h>
#include <gint/clock.h>
#include <gint/mpu/dma.h>
#include <gint/drivers/keydev.h>
#include <gint/defs/attributes.h>
#include <gint/hardware.h>
#include <gint/gint.h>
#include <gint/config.h>
#include <stdlib.h>
#include <string.h>

void __Reset(void);

#define dprint(x, y, ...) dprint(x, y, C_BLACK, __VA_ARGS__)
#define dtext(x, y, str)  dtext (x, y, C_BLACK, str)

/* Weak implementation of driver functions, which are used if the keyboard and
   display drivers are not linked in. This allows add-ins to not link in these
   drivers (which is useful for low-level debugging). */
GWEAK void _WEAK_dupdate(void)
{
	return;
}
GWEAK keydev_t *_WEAK_keydev_std(void)
{
	return NULL;
}
GWEAK bool _WEAK_keydev_keydown(GUNUSED keydev_t *d, GUNUSED int key)
{
	return false;
}
GWEAK key_event_t _WEAK_keydev_unqueue_event(GUNUSED keydev_t *d)
{
	return (key_event_t){ .type = KEYEV_NONE };
}

/* gint_panic_default(): Default panic handler */
GNORETURN static void gint_default_panic(GUNUSED uint32_t code)
{
	uint32_t TEA, TRA;
	keydev_t *kd = _WEAK_keydev_std();

	if(isSH3())
	{
		TEA = *(volatile uint32_t *)0xfffffffc;
		TRA = *(volatile uint32_t *)0xffffffd0 >> 2;
	}
	else
	{
		TEA = *(volatile uint32_t *)0xff00000c;
		TRA = *(volatile uint32_t *)0xff000020 >> 2;
	}

	uint32_t PC;
	__asm__("stc spc, %0" : "=r"(PC));

	uint32_t SGR = 0xffffffff;
	if(isSH4()) __asm__("stc sgr, %0" : "=r"(SGR));

	dfont(NULL);

	#if GINT_RENDER_MONO
	memset(gint_vram, 0, 1024);
	dtext(1, 0, "Exception! (SysERROR)");
	for(int i = 0; i < 32; i++) gint_vram[i] = ~gint_vram[i];

	char const *name = "";
	if(code == 0x040) name = "TLB miss read";
	if(code == 0x060) name = "TLB miss write";
	if(code == 0x0e0) name = "Read address error";
	if(code == 0x100) name = "Write address error";
	if(code == 0x160) name = "Unconditional trap";
	if(code == 0x180) name = "Illegal instruction";
	if(code == 0x1a0) name = "Illegal delay slot";

	/* Custom gint codes for convenience */
	if(code == 0x1020) name = "DMA address error";
	if(code == 0x1040) name = "Add-in too large";
	if(code == 0x1060) name = "Memory init failed";
	if(code == 0x1080) name = "Stack overflow";
	if(code == 0x10a0) name = "UBC in bank 1 code";

	if(name[0]) dtext(1, 9, name);
	else dprint(1, 9, "%03x", code);

	dprint(1, 17, " PC:%08x  TRA:%d", PC, TRA);
	dprint(1, 25, "TEA:%08x", TEA);

	dtext(1, 33, "The add-in crashed.");
	if(kd == NULL) {
		dtext(1, 41, "Please reset the calc");
	}
	else {
		dtext(1, 41, "[EXIT]:Quit add-in");
		dtext(1, 49, "[MENU]:Main menu");
		dtext(1, 57, "[F1]+[F6]: RESET calc");
	}
	#endif

	#if GINT_RENDER_RGB
	/* Don't require the DMA driver just for a clear */
	memset(gint_vram, 0xff, DWIDTH*DHEIGHT*2);
	dtext(6, 3, "An exception occured! (System ERROR)");

	uint32_t *long_vram = (void *)gint_vram;
	for(int i = 0; i < (DWIDTH/2) * 16; i++) long_vram[i] = ~long_vram[i];

	char const *name = "";
	if(code == 0x040) name = "TLB miss (nonexisting address) on read";
	if(code == 0x060) name = "TLB miss (nonexisting address) on write";
	if(code == 0x0e0) name = "Read address error (probably alignment)";
	if(code == 0x100) name = "Write address error (probably alignment)";
	if(code == 0x160) name = "Unconditional trap";
	if(code == 0x180) name = "Illegal instruction";
	if(code == 0x1a0) name = "Illegal delay slot instruction";

	/* Custom gint codes for convenience */
	if(code == 0x1020) name = "DMA address error";
	if(code == 0x1040) name = "Add-in not fully mapped (too large)";
	if(code == 0x1060) name = "Memory initialization failed (heap)";
	if(code == 0x1080) name = "Stack overflow during world switch";
	if(code == 0x10a0) name = "UBC break in register bank 1 code";

	dprint(6, 25, "%03x %s", code, name);

	dtext(6, 45, "PC");
	dprint(38, 45, "= %08x", PC);
	dtext(DWIDTH-135, 45, "(Error location)");

	dtext(6, 60, "TEA");
	dprint(38, 60, "= %08x", TEA);
	dtext(DWIDTH-162, 60, "(Offending address)");

	dtext(6, 75, "TRA");
	dprint(38, 75, "= %#x", TRA);
	dtext(DWIDTH-115, 75, "(Trap number)");

	dtext(6, 95,  "The add-in crashed!");
	if(kd == NULL) {
#if GINT_HW_CG
		dtext(6, 108, "Please press the RESET button to restart the");
		dtext(6, 121, "calculator.");
#else
		dtext(6, 108, "Please press the RESET button to");
		dtext(6, 121, "restart the calculator.");
#endif
	}
	else {
		dtext(6, 121, "[EXIT]: Exit the program with abort()");
		dtext(6, 134, "[MENU]: Leave to main menu");
		dtext(6, 147, "[F1]+[F6]: RESET the calculator");
	}

	/* DMA address error handler */
	if(code == 0x1020)
	{
		#define DMA SH7305_DMA
		dprint(6, 167, "SAR0: %08x   DAR0: %08x   TCR0: %08x",
			DMA.DMA0.SAR, DMA.DMA0.DAR, DMA.DMA0.TCR);
		dprint(6, 180, "CHCR0: %08x", DMA.DMA0.CHCR);
		dprint(6, 193, "SAR1: %08x   DAR1: %08x   TCR1: %08x",
			DMA.DMA1.SAR, DMA.DMA1.DAR, DMA.DMA1.TCR);
		dprint(6, 206, "CHCR1: %08x  DMAOR:%04x", DMA.DMA1.CHCR, DMA.OR);
		#undef DMA
	}
	/* Illegal instruction handler */
	if(code == 0x180)
	{
		uint16_t *opcodes = (void *)PC;
		dprint(6, 160, "Opcodes: %04x %04x [%04x] %04x",
			opcodes[-2], opcodes[-1], opcodes[0], opcodes[1]);
	}
	#endif

	_WEAK_dupdate();

	/* Make sure relevant keys are released before taking in events; we don't
	   want the user to RESET through this screen by holding a key */
	bool has_released = false;

	while(1) {
		while(_WEAK_keydev_unqueue_event(kd).type != KEYEV_NONE) {}

		bool exit = _WEAK_keydev_keydown(kd, KEY_EXIT);
		bool menu = _WEAK_keydev_keydown(kd, KEY_MENU);
		bool f1   = _WEAK_keydev_keydown(kd, KEY_F1);
		bool f6   = _WEAK_keydev_keydown(kd, KEY_F6);

		if(has_released && exit)
			abort();
		if(has_released && menu)
			gint_osmenu();
		if(has_released && f1 && f6)
			__Reset();
		if(!exit && !menu && !f1 && !f6)
			has_released = true;

		sleep();
	}
}

/* Panic handler */
GNORETURN void (*gint_exc_panic)(uint32_t code) = gint_default_panic;
/* Exception catcher */
int (*gint_exc_catcher)(uint32_t code) = NULL;

/* gint_panic(): Panic handler function */
void gint_panic(uint32_t code)
{
	gint_exc_panic(code);
}

/* gint_panic_set(): Change the panic handler function */
void gint_panic_set(GNORETURN void (*panic)(uint32_t code))
{
	gint_exc_panic = panic ? panic : gint_default_panic;
}

/* gint_exc_catch(): Set a function to catch exceptions */
void gint_exc_catch(int (*handler)(uint32_t code))
{
	gint_exc_catcher = handler;
}

/* gint_exc_skip(): Skip pending exception instructions */
void gint_exc_skip(int instructions)
{
	uint32_t spc;

	/* Increase SPC by 2 bytes per instruction */
	__asm__("stc spc, %0" : "=r"(spc));
	spc += 2 * instructions;
	__asm__("ldc %0, spc" :: "r"(spc));
}
