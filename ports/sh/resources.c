//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "resources.h"
#include "debug.h"

/*** Timers ***/

/* There should be at most 32 timers, which is fine. */
static uint32_t timer_used = 0;
#if PE_DEBUG
static uint32_t timer_warn = 0;
#endif

int pe_timer_configure(int timer, uint64_t delay, gint_call_t call, bool waf)
{
    int t = timer_configure(timer, delay, call);
    if(t < 0)
        return t;
    if(t >= 32) {
        timer_stop(t);
        return -1;
    }

    timer_used |= (1 << t);
#if PE_DEBUG
    if(waf)
        timer_warn |= (1 << t);
#endif

    return t;
}

void pe_timer_stop(int t)
{
    if(t < 0 || t >= 32)
        return;

#if PE_DEBUG
    if(!(timer_used & (1 << t)))
        pe_debug_printf("autofree: bad timer %d\n", t);
    timer_warn &= ~(1 << t);
#endif

    timer_used &= ~(1 << t);
}

static void pe_timer_autofree(void)
{
    for(int t = 0; t < 32; t++) {
        if(!(timer_used & (1 << t)))
            continue;
#if PE_DEBUG
        if(timer_warn & (1 << t))
            pe_debug_printf("autofree: timer %d\n", t);
#endif
        timer_stop(t);
    }
}

/*** Autofree function ***/

void pe_resources_autofree(void)
{
    pe_timer_autofree();
}
