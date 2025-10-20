//---
//	gint:timer - Timer operation
//---

#ifndef GINT_TIMER
#define GINT_TIMER

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/mpu/tmu.h>
#include <gint/hardware.h>
#include <gint/defs/types.h>
#include <gint/defs/call.h>

/* Timer types and numbers

   If you're new to timers, read this comment and then check timer_configure()
   and timer_start(): most of the time you only need these.

   There are two types of timers on the calculator: normal timers called TMU,
   and extra timers added by Casio called ETMU. The main difference is that TMU
   are very precise (about 4 MHz; the resolution is about 250 ns) while ETMU
   are much less precise (32 kHz; the resolution is about 30 µs).

   The number of available timers also depends on the platform:
   * SH3-based fx9860g have 3 TMU (ID 0,1,2) and 1 ETMU (ID 3)
   * SH4-based fx9860g and fxcg50 have 3 TMU (ID 0,1,2) and 6 ETMU (ID 3..8)

   You can request "a" timer with timer_configure(), and gint will choose an
   available timer depending on the precision you requested. Or, if you want
   one specifically, you ask for an ID of your choice.

   gint uses 1 to 2 timers by default:
   * One for the keyboard on all calculators, always an ETMU
   * The gray engine on fx9860g uses TMU0 for interrupt priority reasons; TMU0
     is normally available for it unless you've used all TMUs at the same time
   libprof also uses a TMU.

   Most of the time you can just use timer_configure() and specify TIMER_ANY.
   If you want to be sure that you have a TMU or an ETMU, you can do so by
   specifying TIMER_TMU or TIMER_ETMU. If you further want to have a specific
   timer with specific settings, then you can:

   * Set a specific ID in timer_configure(), in which case the delay is no
     longer interpreter as count of µs, but as a TCOR value.
   * If this ID is a TMU, you can further add (with + or |) a prescaler
     specification, one of TIMER_Pphi_{4,16,64,256}.
   * Regardless of how the timer was obtained, you can use timer_reload() to
     replace the value of TCOR.
   * Also note that TMU0, TMU1, TMU2 and the ETMU have respective interrupt
     priority levels of 13, 11, 9, and 7. The gray engine uses TMU0 to
     guarantee maximum visual stability in the presence of interrupts.

   In this module, timers are manipulated through their ID. timer_configure()
   returns the ID of a timer which was allocated to you. You can check it to
   determine whether your timer is a TMU (0,1,2) or an ETMU (3 or more). */

/* timer_count(): Number of timers available on the platform */
#define timer_count() (isSH3() ? 4 : 9)

/* Clock input

   Standard TMU can count at different speeds. A fast speed offers more
   precision but a slower speed offers longer delays. gint automatically
   selects suitable speeds by default.

   If you want something very particular, you can add (with + or |) a prescaler
   value to a chosen ID in timer_configure() to request that specific value.
   The default prescaler if the ID is fixed is TIMER_Pphi_4. */
enum {
	TIMER_Pphi_4   = 0x00,
	TIMER_Pphi_16  = 0x10,
	TIMER_Pphi_64  = 0x20,
	TIMER_Pphi_256 = 0x30,
};

/* Timer selection; see timer_configure() */
enum {
	TIMER_ANY  = -1,
	TIMER_TMU  = -2,
	TIMER_ETMU = -3,
};

/* Return value for timer callbacks, indicating whether the timer should
   continue running and fire again, or stop now */
enum {
	TIMER_CONTINUE = 0,
	TIMER_STOP     = 1,
};

//---
//	Timer functions
//---

/* timer_configure(): Reserve and configure a timer

   This function finds and configures a timer (without starting it). On
   success, it returns the ID of the configured timer, which is used in all
   other timer functions. If no timer matching the requested settings is
   available, this function returns -1.

   When started, the configured timer will run for the requested delay and call
   the supplied callback function at the end of this delay. The callback
   function can then decide whether to leave the timer running (and be called
   again after the same delay) or stop the timer.

   The first argument specifies what kind of timer you want.
   * TIMER_ANY will let timer_configure() choose any available timer.
     timer_configure() will only use an ETMU if the delay is more than 0.1 ms
     to avoid resolution issues. Most of the time this is what you need.
   * TIMER_TMU or TIMER_ETMU will let timer_configure() choose an available TMU
     or ETMU, respectively.
   * Specifying an ID (0..8) will request exactly that timer. In this case, and
     if the ID is a TMU (0,1,2), you may add (with + or |) a prescaler value to
     specify the clock input. Otherwise the clock is set to Pphi/4.
   If no timer matching the supplied settings is available, timer_configure()
   returns -1.

   The second argument is the delay. With TIMER_ANY, TIMER_TMU and TIMER_ETMU,
   the delay is interpreted as a number of µs. With an explicit ID, the delay
   is interpreted as a value of TCOR; see timer_delay() in this case. Note that
   TCOR values are sensitive to the overclock level!

   The third argument is an indirect function call to be made when the timer
   expires. You can create one with GINT_CALL(); the function must return an
   int equal to either TIMER_CONTINUE or TIMER_STOP to control the subsequent
   operation of the timer. Default calls are available in <gint/defs/call.h>
   with common operations that are useful in timers. If GINT_CALL_NULL is
   passed, a default function that stops the timer will be used.

   On success, the configured timer becomes reserved; it can no longer be
   returned by timer_configure() until:
   * Either timer_stop() is called,
   * Or the callback returns TIMER_STOP (which also stops the timer).
   Remember than the returned timer is not started yet; see timer_start().

   @timer     Requested timer; TIMER_{ANY,TMU,ETMU} or an ID with prescaler
   @delay     Delay between each event, in µs unless first argument is an ID
   @callback  Function to be called when the timer fires */
int timer_configure(int timer, uint64_t delay_us, gint_call_t callback);

/* timer_start(): Start a configured timer
   The specified timer will start counting down and call its callback function
   at regular intervals until it is paused or stopped. */
void timer_start(int timer);

/* timer_pause(): Pause a timer without freeing it
   Pauses the specified timer will be paused. The timer stops counting but is
   not freed and can be resumed by calling timer_start(). */
void timer_pause(int timer);

/* timer_stop(): Stop and free a timer
   Stops and frees a timer, making it available to timer_setup(). This timer
   should no longer be used unless it is returned again by timer_setup(). */
void timer_stop(int timer);

/* timer_wait(): Wait for a timer to stop
   Waits until the timer pauses or stops. If the timer is not running, returns
   immediately. Even after timer_wait(), the timer may not be available since
   it may have only paused. If the timer never stops, you're in trouble. */
void timer_wait(int timer);

/* timer_spinwait(): Start a timer and actively wait for UNF
   Waits until the timer raises UNF, without sleeping. This is useful for
   delays in driver code that is run when interrupts are disabled. UNIE is
   disabled before starting the timer and waiting, so the callback is never
   called. */
void timer_spinwait(int timer);

//---
//	Low-level functions
//---

/* timer_delay(): Compute a timer constant from a duration in seconds

   This function calculates the timer constant for a given delay. timer_setup()
   does this computation when TIMER_ANY, TIMER_TMU or TIMER_ETMU is provided,
   but expects you to provide the exact constant when an explicit timer ID is
   requested. timer_reload() also expects the constant.

   For TMU the clock can be Pphi/4, Pphi/16, Pphi/64 and Pphi/256, which can
   respectively count up to about 350 seconds, 23 minutes, 95 minutes and 381
   minutes.

   For ETMU the clock is TCLK at 32768 Hz, which can count up to 36 hours. Any
   longer delay should probably be managed by the RTC (which counts even when
   the calculator is off).

   @timer     The timer you are planning to use
   @delay_us  Requested delay in microseconds
   @clock     The clock value, irrelevant if timer >= 3 */
uint32_t timer_delay(int timer, uint64_t delay_us, int clock);

/* timer_reload(): Change a timer's delay constant for next interrupts

   Changes the delay constant of the given timer. Nothing will happen until the
   next callback; then the timer will update its delay to reflect the new
   constant. The new delay can be calculated by the timer_delay() function.

   @timer  Timer id, as returned by timer_setup()
   @delay  New delay (unit depends on the clock source) */
void timer_reload(int timer, uint32_t delay);

//---
// Deprecated API
//
// These types and functions were used in previous versions of gint but have
// been replaced. They are still here for compatibility until gint 3.
//---

/* Type of callback functions; a primitive predecessor to GINT_CALL() */
typedef union
{
   /* No argument, returns either TIMER_CONTINUE or TIMER_STOP */
   int (*v)(void);
   /* Single integer argument */
   int (*i)(int);
   /* Single pointer argument, cv-qualified as needed */
   int (*pv)  (void *);
   int (*pVv) (volatile void *);
   int (*pCv) (const void *);
   int (*pCVv)(volatile const void *);
   /* Integer pointer argument, cv-qualified as needed */
   int (*pi)  (int *);
   int (*pVi) (volatile int *);
   int (*pCi) (const int *);
   int (*pCVi)(volatile const int *);

} GTRANSPARENT timer_callback_t;

/* timer_setup(): Old variant of timer_configure() */
__attribute__((deprecated("Use timer_configure() instead")))
int timer_setup(int timer, uint64_t delay_us, timer_callback_t callback, ...);

/* Makes sure an argument is always provided, for va_arg() */
#define timer_setup(...) timer_setup(__VA_ARGS__, 0)

/* timer_timeout(): Callback that sets a flag and halts the timer
   Use GINT_CALL_SET_STOP() with timer_configure() instead. */
int timer_timeout(volatile void *arg);

#ifdef __cplusplus
}
#endif

#endif /* GINT_TIMER */
