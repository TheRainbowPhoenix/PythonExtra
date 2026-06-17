#include <gint/usb.h>
#include <gint/mpu/usb.h>
#include <gint/mpu/power.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/pfc.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/intc.h>
#include <gint/cpu.h>
#include "usb_private.h"

#define USB SH7305_USB

static void usb_interrupt_handler(gint_inth_callback_context_t* interrupt_context);

/* Shorthand to clear a bit in INTSTS0 */
#define INTSTS0_clear(field_name) {				\
	__typeof__(USB.INTSTS0) __intsts0 = { .word = 0xffff };	\
	__intsts0.field_name = 0;				\
	do USB.INTSTS0 = __intsts0;				\
	while(USB.INTSTS0.field_name != 0);			\
}

/* Callback function to invoke when the USB module is configured */
/* TODO: usb_open() callback: Let interfaces specify when they're ready! */
static gint_call_t usb_open_callback = GINT_CALL_NULL;
/* Whether the USB link is currently open */
static bool volatile usb_open_status = false;

//---
// Debugging functions
//---

static void (*usb_logger)(char const *format, va_list args) = NULL;
static void (*usb_tracer)(char const *message) = NULL;

void usb_set_log(void (*logger)(char const *format, va_list args))
{
	usb_logger = logger;
}

void (*usb_get_log(void))(char const *format, va_list args)
{
	return usb_logger;
}

void usb_log(char const *format, ...)
{
	if(!usb_logger) return;
	va_list args;
	va_start(args, format);
	usb_logger(format, args);
	va_end(args);
}

void usb_set_trace(void (*tracer)(char const *message))
{
	usb_tracer = tracer;
}

void (*usb_get_trace(void))(char const *message)
{
	return usb_tracer;
}

void usb_trace(char const *message)
{
	if(usb_tracer) {
		cpu_atomic_start();
		usb_tracer(message);
		cpu_atomic_end();
	}
}

//---
// Module powering and depowering
//---

static bool hpowered(void)
{
	return (SH7305_CPG.USBCLKCR.CLKSTP == 0) &&
		(SH7305_POWER.MSTPCR2.USB0 == 0);
}

static void hpoweron(void)
{
	if(hpowered()) return;

	SH7305_PFC.MSELCRA.UNKNOWN_USB = 0;
	SH7305_PFC.MSELCRB.XTAL_USB = 0;

	/* Leave some delay for the clock to settle. The OS leaves
	   100 ms, but it just never seems necessary. */
	SH7305_CPG.USBCLKCR.CLKSTP = 0;
	sleep_us_spin(1000);

	SH7305_POWER.MSTPCR2.USB0 = 0;
	SH7305_USB_UPONCR.word = 0x0600;
}

/* Finish the poweron procedure by enabling writes in the registers */
static void hpoweron_write(void)
{
	/* Turn on SCKE, which activates all other registers. The existing
	   BUSWAIT delay might not be high enough, so wait a little bit before
	   modifying registers; a couple CPU cycles is enough. */
	USB.SYSCFG.SCKE = 1;
	for(int i = 0; i < 10; i++) __asm__ volatile("nop");

	/* Set BUSWAIT to a safe value */
	USB.BUSWAIT.word = 5;
}

static void hpoweroff(void)
{
	SH7305_USB_UPONCR.word = 0x0000;

	/* This delay is crucial and omitting it has caused constant freezes in
	   the past. Blame undocumented clock magic? */
	sleep_us_spin(1000);
	SH7305_POWER.MSTPCR2.USB0 = 1;

	SH7305_CPG.USBCLKCR.CLKSTP = 1;
	sleep_us_spin(1000);

	/* The values used by the OS (a PFC driver could do better) */
	SH7305_PFC.MSELCRB.XTAL_USB = 3;
	SH7305_PFC.MSELCRA.UNKNOWN_USB = 1;
}

int usb_open(usb_interface_t const **interfaces, gint_call_t callback)
{
	if(usb_open_status)
		return USB_OPEN_ALREADY_OPEN;

	/* TODO: Check whether the calculator can host devices (probably no) */
	bool host = false;

	USB_LOG("---- usb_open ----\n");

	int rc = usb_configure_solve(interfaces);
	usb_configure_log();

	if(rc != 0)
	{
		USB_LOG("configure failure: %d\n", rc);
		return rc;
	}

	usb_open_callback = callback;
	if(!hpowered()) hpoweron();
	hpoweron_write();

	USB.REG_C2 = 0x0020;

	/* Disconnect (DPRPU=0) if we were previously connected as a function.
	   Also drive down DRPD, since both are required for setting DCFM. */
	USB.SYSCFG.DPRPU = 0;
	USB.SYSCFG.DRPD = 0;

	if(host) {
		/* Select the host controller */
		USB.SYSCFG.DCFM = 1;
		/* Clear registers to prepare for host operation */
		USB.SYSCFG.USBE = 0;
		/* Accept both high-speed and full-speed devices */
		USB.SYSCFG.HSE = 0;

		/* Pull DPRD and eliminate chattering */
		USB.SYSCFG.DRPD = 1;
		GUNUSED volatile int LNST = USB.SYSSTS.LNST;

		/* Enable the module */
		USB.SYSCFG.USBE = 1;
	}
	else {
		/* Select the function controller */
		USB.SYSCFG.DCFM = 0;
		/* Clear registers to prepare for function operation */
		USB.SYSCFG.USBE = 0;
		/* Use high-speed only */
		USB.SYSCFG.HSE = 1;

		/* Enable the module */
		USB.SYSCFG.USBE = 1;
	}

	usb_pipe_reset_fifos();
	for(int i = 0; i <= 9; i++)
		usb_pipe_reset(i);
	usb_pipe_init_transfers();

	/* Prepare the default control pipe. */
	USB.DCPCFG.DIR = 0;
	USB.DCPMAXP.DEVSEL = 0;
	USB.DCPMAXP.MXPS = 64;

	USB.SOFCFG.enable = 1;

	/* Configure CFIFOSEL to use the DCP */
	USB.CFIFOSEL.RCNT = 0;
	USB.CFIFOSEL.REW = 0;
	USB.CFIFOSEL.BIGEND = 1;

	/* VBSE=1 RSME=0 SOFE=0 DVSE=1 CTRE=1 BEMPE=1 NRDYE=0 BRDYE=1 */
	USB.INTENB0.word = 0x9d00;
	USB.INTENB1.word = 0x0000;
	USB.BRDYENB = 0x0000;
	USB.NRDYENB = 0x0000;
	USB.BEMPENB = 0x0000;
	USB.BRDYSTS = 0x0000;
	USB.NRDYSTS = 0x0000;
	USB.BEMPSTS = 0x0000;

	intc_handler_function(0xa20, GINT_CALL_FLAG(usb_interrupt_handler));
	intc_priority(INTC_USB, 8);

	/* Pull D+ up to 3.3V, notifying connection when possible. Read
	   SYSSTS.LNST as required after modifying DPRPU */
	USB.SYSCFG.DPRPU = 1;
	GUNUSED volatile int LNST = USB.SYSSTS.LNST;

	return 0;
}

bool usb_is_open(void)
{
	return usb_open_status;
}

bool usb_is_open_interface(usb_interface_t const *interface)
{
	if(!usb_is_open())
		return false;

	usb_interface_t const * const *open = usb_configure_interfaces();
	for(int i = 0; open[i]; i++) {
		if(open[i] == interface)
			return true;
	}
	return false;
}

void usb_open_wait(void)
{
	while(!usb_open_status) sleep();
}

void usb_close(void)
{
	usb_wait_all_transfers(false);
	usb_pipe_init_transfers();

	intc_priority(INTC_USB, 0);
	hpoweroff();
	USB_LOG("---- usb_close ----\n");

	usb_open_callback = GINT_CALL_NULL;
	usb_open_status = false;
}

//---
// Userspace interrupt handler
//---

gint_inth_callback_context_t* usb_interrupt_context;

static void usb_interrupt_handler(gint_inth_callback_context_t* interrupt_context)
{
	usb_interrupt_context = interrupt_context;

	GUNUSED static char const * const device_st[] = {
		"powered", "default", "address", "configured",
		"suspended-powered", "suspended-default", "suspended-address",
		"suspended-configured",
	};
	/* Save PIPESEL to avoid concurrent access issues */
	uint16_t pipesel = USB.PIPESEL.word;

	if(USB.INTSTS0.VBINT)
	{
		INTSTS0_clear(VBINT);
		USB_LOG("VBUS %s\n", USB.INTSTS0.VBSTS ? "up" : "down");
	}
	else if(USB.INTSTS0.CTRT)
	{
		INTSTS0_clear(CTRT);
		if(USB.INTSTS0.VALID) usb_req_setup();
	}
	else if(USB.INTSTS0.DVST)
	{
		INTSTS0_clear(DVST);
		USB_LOG("DVST %s", device_st[USB.INTSTS0.DVSQ]);
		if(USB.INTSTS0.DVSQ == 2) USB_LOG(": %04x\n",USB.USBADDR.word);
		else USB_LOG("\n");

		/* When configured, run the callback for usb_open() */
		if(USB.INTSTS0.DVSQ == 3)
		{
			usb_configure();

			usb_open_status = true;
			gint_call(usb_open_callback);
			usb_open_callback = GINT_CALL_NULL;
		}
	}
	else if(USB.INTSTS0.BEMP)
	{
		/* Invoke callbacks for each buffer-empty interrupt */
		uint16_t status = USB.BEMPSTS & USB.BEMPENB;
		USB.BEMPSTS = 0;

		for(int i = 0; i <= 9; i++)
		{
			if(status & (1 << i))
				usb_pipe_write_bemp(i);
		}
	}
	else if(USB.INTSTS0.BRDY)
	{
		uint16_t status = USB.BRDYSTS & USB.BRDYENB;
		USB.BRDYSTS = 0;

		for(int i = 0; i <= 9; i++)
		{
			if(status & (1 << i))
				usb_pipe_read_brdy(i);
		}
	}
	else USB_LOG("<%04X> -> ???\n", USB.INTSTS0.word);

	/* Restore PIPESEL which can have been used for transfers */
	USB.PIPESEL.word = pipesel;

	usb_interrupt_context = NULL;
}

//---
// State and driver metadata
//---

void hsave(usb_state_t *s)
{
	s->SYSCFG    = USB.SYSCFG.word;
	s->BUSWAIT   = USB.BUSWAIT.word;

	s->DVSTCTR   = USB.DVSTCTR.word;
	s->SOFCFG    = USB.SOFCFG.word;
	s->TESTMODE  = USB.TESTMODE.word;
	s->REG_C2    = USB.REG_C2;

	s->INTENB0   = USB.INTENB0.word;
	s->INTENB1   = USB.INTENB1.word;
	s->BRDYENB   = USB.BRDYENB;
	s->NRDYENB   = USB.NRDYENB;
	s->BEMPENB   = USB.BEMPENB;

	s->DCPMAXP   = USB.DCPMAXP.word;

#ifdef GINT_USB_DEBUG
	/* Save some other state for inspection only. Since we need to write to
	   PIPESEL in order to read PIPECFG etc, enable writing. */
	hpoweron_write();

	s->SYSSTS    = USB.SYSSTS.word;
	s->FRMNUM    = USB.FRMNUM.word;
	s->UFRMNUM   = USB.UFRMNUM.word;

	s->CFIFOSEL  = USB.CFIFOSEL.word;
	s->D0FIFOSEL = USB.D0FIFOSEL.word;
	s->D1FIFOSEL = USB.D1FIFOSEL.word;
	s->CFIFOCTR  = USB.CFIFOCTR.word;
	s->D0FIFOCTR = USB.D0FIFOCTR.word;
	s->D1FIFOCTR = USB.D1FIFOCTR.word;

	s->INTSTS0   = USB.INTSTS0.word;
	s->INTSTS1   = USB.INTSTS1.word;
	s->BRDYSTS   = USB.BRDYSTS;
	s->NRDYSTS   = USB.NRDYSTS;
	s->BEMPSTS   = USB.BEMPSTS;

	s->DCPCFG    = USB.DCPCFG.word;
	s->DCPCTR    = USB.DCPCTR.word;

	s->USBADDR   = USB.USBADDR.word;
	s->USBREQ    = USB.USBREQ.word;
	s->USBVAL    = USB.USBVAL.word;
	s->USBINDX   = USB.USBINDX.word;
	s->USBLENG   = USB.USBLENG.word;

	s->PIPESEL   = USB.PIPESEL.word;
	for(int i = 0; i < 9; i++) {
		USB.PIPESEL.PIPESEL = i+1;
		s->PIPECFG[i]   = USB.PIPECFG.word;
		s->PIPEnCTR[i]  = USB.PIPECTR[i].word;
		s->PIPEBUF[i]   = USB.PIPEBUF.word;
	}
#endif

	/* Leave the module open for gint to use it, or for the next restore
	   to proceed more quickly (if during a world switch) */
}

static void hrestore(usb_state_t const *s)
{
	hpoweron_write();

	/* We will need to reconnect with the PC */
	usb_open_status = false;

	USB.DVSTCTR.word        = s->DVSTCTR;
	USB.SOFCFG.word         = s->SOFCFG;
	USB.TESTMODE.word       = s->TESTMODE;
	USB.FRMNUM.word         = 0;
	USB.REG_C2              = s->REG_C2;

	USB.INTENB0.word        = s->INTENB0;
	USB.INTENB1.word        = s->INTENB1;
	USB.BRDYENB             = s->BRDYENB;
	USB.NRDYENB             = s->NRDYENB;
	USB.BEMPENB             = s->BEMPENB;

	usb_pipe_reset_fifos();
	for(int i = 0; i <= 9; i++)
		usb_pipe_reset(i);

	USB.DCPMAXP.word        = s->DCPMAXP;
	USB.PIPESEL.word        = 0x0000;

	/* Clear remaining interrupts. Read-only bits will be ignored */
	USB.INTSTS0.word        = 0x0000;
	USB.INTSTS1.word        = 0x0000;
	USB.BRDYSTS             = 0x0000;
	USB.NRDYSTS             = 0x0000;
	USB.BEMPSTS             = 0x0000;

	/* Restore SYSCFG last as clearing SCKE disables writing */
	USB.BUSWAIT.word        = s->BUSWAIT;
	USB.SYSCFG.word         = s->SYSCFG;
}

gint_driver_t drv_usb = {
	.name         = "USB",
	/* TODO: usb: Wait for remaining transfers in unbind() */
	.hpowered     = hpowered,
	.hpoweron     = hpoweron,
	.hpoweroff    = hpoweroff,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(usb_state_t),
};
GINT_DECLARE_DRIVER(16, drv_usb);
