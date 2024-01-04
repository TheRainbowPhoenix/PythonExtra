//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.mphalport: MicroPython's Hardware Abstraction Layer

#include <gint/clock.h>
#include <time.h>
#include "shared/runtime/interrupt_char.h"
#include "py/misc.h"

/* We don't use a VT100 terminal. */
#define MICROPY_HAL_HAS_VT100 (0)
/* Use our custom readline for input(). */
int pe_readline(vstr_t *line, char const *prompt);
#define mp_hal_readline pe_readline

/* Passive sleep. */
static inline void mp_hal_delay_ms(mp_uint_t ms)
{
    sleep_ms(ms);
}
static inline void mp_hal_delay_us(mp_uint_t us)
{
    sleep_us(us);
}

/* Time spent executing. */
static inline mp_uint_t mp_hal_ticks_ms(void)
{
    return ((uint64_t)clock() * 1000) / CLOCKS_PER_SEC;
}
static inline mp_uint_t mp_hal_ticks_us(void)
{
    return ((uint64_t)clock() * 1000000) / CLOCKS_PER_SEC;
}
static inline mp_uint_t mp_hal_ticks_cpu(void)
{
    return clock();
}

/* Time since Epoch in nanoseconds. */
static inline uint64_t mp_hal_time_ns(void)
{
    return (uint64_t)time(NULL) * 1000000000;
}
