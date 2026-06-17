//---
// gint:defs:timeout - RTC-based timeouts
//
// This header provides an interface for simplistic timers used for timeout
// waiting. Currently they are based on the RTC with a resolution of 1/128 s.
//---

#ifndef GINT_DEFS_TIMEOUT
#define GINT_DEFS_TIMEOUT

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdbool.h>

/* Object holding information about a timeout (specifically, when it expires).
   TODO: timeout: consider using a struct timespec with clock_gettime()? */
typedef clock_t timeout_t;

static inline timeout_t timeout_make_ms(int ms)
{
    return clock() + (int64_t)ms * CLOCKS_PER_SEC / 1000;
}

static inline bool timeout_elapsed(timeout_t const *t)
{
    return t && clock() >= *t;
}

#ifdef __cplusplus
}
#endif

#endif /* GINT_DEFS_TIMEOUT */
