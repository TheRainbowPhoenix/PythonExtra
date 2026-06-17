//---
//	gint:tmu - Timer operation
//---

#include <gint/timer.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/intc.h>
#include <gint/cpu.h>
#include <gint/mpu/tmu.h>
#include <stdarg.h>

/* Callbacks for all timers */
gint_call_t tmu_callbacks[9];

/* Arrays of standard and extra timers */
static tmu_t *TMU = SH7305_TMU.TMU;
static etmu_t *ETMU = SH7305_ETMU;

/* TSTR register for standard timers */
static volatile uint8_t *TSTR = &SH7305_TMU.TSTR;

/* Shortcut to set registers that are slow to update */
#define set(lval, rval) do(lval = rval); while(rval != lval)

//---
//	Local functions
//---

/* conf(): Configure a fixed timer */
static void conf(int id, uint32_t delay, int clock, gint_call_t call)
{
	if(id < 3)
	{
		/* Refuse to setup timers that are already in use */
		tmu_t *T = &TMU[id];
		if(T->TCR.UNIE || *TSTR & (1 << id)) return;

		/* Configure the counter, clear interrupt flag*/
		T->TCOR = delay;
		T->TCNT = delay;
		T->TCR.TPSC = clock;
		set(T->TCR.UNF, 0);

		/* Enable interrupt and count on rising edge (SH7705) */
		T->TCR.UNIE = 1;
		T->TCR.CKEG = 0;
	}
	else
	{
		etmu_t *T = &ETMU[id-3];
		if(T->TCR.UNIE) return;

		/* No clock input and clock edge here */
		set(T->TCR.UNF, 0);
		set(T->TCOR, delay);
		set(T->TCNT, delay);
		T->TCR.UNIE = 1;
	}

	tmu_callbacks[id] = call;
}

/* matches(): Check if a timer matches the provided specification and delay */
static int matches(int id, int spec, uint64_t delay)
{
	/* A specific idea only matches the exact timer */
	if(spec >= 0) return id == (spec & 0xf);
	/* TIMER_ANY always matches ETMU only for delays at least 100 µs */
	if(spec == TIMER_ANY) return (id < 3 || delay >= 100);
	/* TIMER_TMU and TIMER_ETMU match as you'd expect */
	if(spec == TIMER_TMU) return (id < 3);
	if(spec == TIMER_ETMU) return (id >= 3);
	/* Default is not matching */
	return 0;
}

/* available(): Check if a timer is available (UNIE cleared, not running) */
static int available(int id)
{
	if(id >= timer_count()) return 0;

	if(id < 3)
	{
		tmu_t *T = &TMU[id];
		return !T->TCR.UNIE && !(*TSTR & (1 << id));
	}
	else
	{
		etmu_t *T = &ETMU[id-3];
		return !T->TCR.UNIE && !T->TSTR;
	}
}

/* stop_callback(): Empty callback that stops the timer */
static int stop_callback(void)
{
	return TIMER_STOP;
}

//---
//	Timer API
//---

int timer_configure(int spec, uint64_t delay, gint_call_t call)
{
	int clock = 0;

	/* Default behavior for the callback */
	if(!call.function) call = GINT_CALL(stop_callback);

	/* Find a matching timer, starting from the slowest timers with the
	   smallest interrupt priorities all the way up to TMU0 */
	for(int id = timer_count() - 1; id >= 0; id--)
	{
		if(!matches(id, spec, delay) || !available(id)) continue;

		/* If ID is a TMU, choose a timer prescaler. Assuming the worst
		   running Pphi of ~48 MHz, select the finest resolution that
		   allows the requested delay to be represented. */
		if(id < 3 && spec >= 0)
		{
			/* Explicit timers with clock in the specification */
			clock = (spec >> 4) & 0xf;
		}
		else if(id < 3)
		{
			uint64_t sec = 1000000;

			/* Pphi/4 until 350 seconds */
			if(delay <= 350 * sec) clock = TIMER_Pphi_4;
			/* Pphi/16 until 1430 seconds */
			else if(delay <= 1430 * sec) clock = TIMER_Pphi_16;
			/* Pphi/64 until 5720 seconds */
			else if(delay <= 5720 * sec) clock = TIMER_Pphi_64;
			/* Pphi/256 otherwise */
			else clock = TIMER_Pphi_256;
		}

		/* Find the delay constant for that timer and clock */
		if(spec < 0) delay = timer_delay(id, delay, clock);

		conf(id, delay, clock, call);
		return id;
	}

	return -1;
}

/* timer_delay() - compute a delay constant from a duration in seconds */
uint32_t timer_delay(int id, uint64_t delay_us, int clock)
{
	uint64_t freq;

	if(id < 3)
	{
		const clock_frequency_t *cpg = clock_freq();
		freq = cpg->Pphi_f;
		if(clock == TIMER_Pphi_4)   freq >>= 2;
		if(clock == TIMER_Pphi_16)  freq >>= 4;
		if(clock == TIMER_Pphi_64)  freq >>= 6;
		if(clock == TIMER_Pphi_256) freq >>= 8;
	}
	else
	{
		/* ETMU all run on TCLK at 32768 Hz */
		freq = 32768;
	}

	uint64_t product = freq * delay_us;
	return product / 1000000;
}

/* timer_control() - start or stop a timer
   @id     Timer ID to configure
   @state  0 to start the timer, 1 to stop it (nothing else!) */
static void timer_control(int id, int state)
{
	if(id < 3) *TSTR = (*TSTR | (1 << id)) ^ (state << id);
	else ETMU[id-3].TSTR = state ^ 1;
}

/* timer_start() - start a configured timer */
void timer_start(int id)
{
	timer_control(id, 0);
}

/* timer_reload() - change a timer's delay constant for next interrupts */
void timer_reload(int id, uint32_t delay)
{
	if(id < 3) TMU[id].TCOR = delay;
	else ETMU[id-3].TCOR = delay;
}

/* timer_pause() - stop a running timer */
void timer_pause(int id)
{
	timer_control(id, 1);
}

/* timer_stop() - stop and free a timer */
void timer_stop(int id)
{
	/* Stop the timer and disable UNIE to indicate that it's free */
	timer_pause(id);

	if(id < 3)
	{
		TMU[id].TCR.UNIE = 0;
		TMU[id].TCR.UNF = 0;
		TMU[id].TCOR = 0xffffffff;
		TMU[id].TCNT = 0xffffffff;
	}
	else
	{
		/* Extra timers generate interrupts when TCNT=0 even if TSTR=0.
		   We always keep TCOR/TCNT to non-zero values when idle. */
		etmu_t *T = &ETMU[id-3];
		T->TCR.UNIE = 0;
		set(T->TCOR, 0xffffffff);
		set(T->TCNT, 0xffffffff);
		set(T->TCR.UNF, 0);
	}
}

/* timer_wait(): Wait for a timer to stop */
void timer_wait(int id)
{
	if(id < 3)
	{
		tmu_t *T = &TMU[id];
		/* Sleep only if an interrupt will be there to wake us up */
		while(*TSTR & (1 << id)) if(T->TCR.UNIE) sleep();
	}
	else
	{
		etmu_t *T = &ETMU[id-3];
		while(T->TSTR) if(T->TCR.UNIE) sleep();
	}
}

/* timer_spinwait(): Start a timer and actively wait for UNF */
void timer_spinwait(int id)
{
	if(id < 3)
	{
		tmu_t *T = &TMU[id];
		T->TCR.UNIE = 0;
		timer_start(id);
		while(!T->TCR.UNF) {}
	}
	else
	{
		etmu_t *T = &ETMU[id-3];
		set(T->TCR.UNIE, 0);
		timer_start(id);
		while(!T->TCR.UNF) {}
	}
}

//---
// Overclock adjustment
//---

void timer_rescale(uint32_t old_Pphi, uint32_t new_Pphi_0)
{
	uint64_t new_Pphi = new_Pphi_0;

	for(int id = 0; id < 3; id++)
	{
		tmu_t *T = &TMU[id];
		/* Skip timers that are not running */
		if(T->TCNT == 0xffffffff && T->TCOR == 0xffffffff)
			continue;

		/* For libprof: keep timers with max TCOR as they are */
		if(T->TCOR != 0xffffffff) {
			T->TCOR = ((uint64_t)T->TCOR * new_Pphi) / old_Pphi;
		}
		T->TCNT = ((uint64_t)T->TCNT * new_Pphi) / old_Pphi;
	}
}

//---
// Deprecated API
//---

#undef timer_setup
int timer_setup(int spec, uint64_t delay, timer_callback_t function, ...)
{
	va_list va;
	va_start(va, function);
	uint32_t arg = va_arg(va, uint32_t);
	va_end(va);

	return timer_configure(spec, delay, GINT_CALL(function.v, arg));
}

int timer_timeout(void volatile *arg)
{
	int volatile *x = arg;
	if(x) (*x)++;
	return TIMER_STOP;
}

//---
//	Driver initialization
//---

/* Interrupt handlers for standard timers (3 gates) */
extern void inth_tmu(void);
/* Interrupt handlers for extra timers */
extern void inth_etmu4(void);
extern void inth_etmux(void);

static void constructor(void)
{
	if(isSH3())
	{
		TMU = SH7705_TMU.TMU;
		ETMU = SH7705_ETMU;
		TSTR = &SH7705_TMU.TSTR;
	}
}

static void configure(void)
{
	uint16_t etmu_event[6] = { 0x9e0, 0xc20, 0xc40, 0x900, 0xd00, 0xfa0 };
	*TSTR = 0;

	/* Install the TMU handlers and adjust the TCR0 value on SH3 */
	void *h = intc_handler(0x400, inth_tmu, 96);
	if(isSH3()) *(void volatile **)(h + 88) = &TMU[0].TCR;

	/* Clear all timers */
	for(int i = 0; i < 3; i++)
	{
		set(TMU[i].TCR.word, 0);
		TMU[i].TCOR = 0xffffffff;
		TMU[i].TCNT = 0xffffffff;
	}
	for(int i = 3; i < timer_count(); i++)
	{
		etmu_t *T = &ETMU[i-3];
		T->TSTR = 0;
		set(T->TCOR, 0xffffffff);
		set(T->TCNT, 0xffffffff);
		set(T->TCR.byte, 0);
	}

	/* Install the ETMU4 handler, which contains the logic for ETMUs */
	void *h4 = intc_handler(etmu_event[4], inth_etmu4, 96);

	/* Install the other ETMU handlers, and set their parameters */
	for(int i = 3; i < timer_count(); i++) if(i != 7)
	{
		void *h = intc_handler(etmu_event[i-3], inth_etmux, 32);

		/* Distance from VBR handler to ETMU4, used to jump */
		*(uint32_t *)(h + 20) += (uint32_t)h4 - cpu_getVBR();
		/* Timer ID, used for timer_stop() after the callback */
		*(uint16_t *)(h + 18) = i;
		/* Pointer to the callback */
		*(gint_call_t **)(h + 24) += i;
		/* TCR address to acknowledge the interrupt */
		*(void volatile **)(h + 28) = &ETMU[i-3].TCR;
	}

	/* Enable TMU0 at level 13, TMU1 at level 11, TMU2 at level 9 */
	intc_priority(INTC_TMU_TUNI0, 13);
	intc_priority(INTC_TMU_TUNI1, 11);
	intc_priority(INTC_TMU_TUNI2, 9);

	/* Enable the extra TMUs at level 7 */
	intc_priority(INTC_ETMU_TUNI0, 7);
	if(isSH4())
	{
		intc_priority(INTC_ETMU_TUNI1, 7);
		intc_priority(INTC_ETMU_TUNI2, 7);
		intc_priority(INTC_ETMU_TUNI3, 7);
		intc_priority(INTC_ETMU_TUNI4, 7);
		intc_priority(INTC_ETMU_TUNI5, 7);
	}
}

//---
// State and driver metadata
//---

static void hsave(tmu_state_t *s)
{
	s->TSTR = *TSTR;

	for(int i = 0; i < 3; i++)
	{
		s->t[i].TCOR = TMU[i].TCOR;
		s->t[i].TCNT = TMU[i].TCNT;
		s->t[i].TCR  = TMU[i].TCR.word;
	}
	for(int i = 3; i < timer_count(); i++)
	{
		struct tmu_state_stored_timer *c = &s->t[i];
		etmu_t *T = &ETMU[i-3];

		/* Don't snapshot an interrupt state, because the timer state
		   is sometimes garbage protected by a masked interrupt. */
		c->TCOR = T->TCOR ? T->TCOR : 0xffffffff;
		c->TCNT = T->TCNT ? T->TCNT : c->TCOR;
		c->TCR  = T->TCR.byte & 0xd;
		c->TSTR	= T->TSTR;
	}
}

static void hrestore(tmu_state_t const *s)
{
	*TSTR = 0;

	for(int i = 0; i < 3; i++)
	{
		TMU[i].TCOR = s->t[i].TCOR;
		TMU[i].TCNT = s->t[i].TCNT;
		TMU[i].TCR.word = s->t[i].TCR;
	}
	for(int i = 3; i < timer_count(); i++)
	{
		struct tmu_state_stored_timer const *c = &s->t[i];
		etmu_t *T = &ETMU[i-3];

		set(T->TCOR, c->TCOR);
		T->TSTR = c->TSTR;
		set(T->TCNT, c->TCNT);
		set(T->TCR.byte, c->TCR);
	}

	*TSTR = s->TSTR;
}

gint_driver_t drv_tmu = {
	.name         = "TMU",
	.constructor  = constructor,
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(tmu_state_t),
};
GINT_DECLARE_DRIVER(13, drv_tmu);
