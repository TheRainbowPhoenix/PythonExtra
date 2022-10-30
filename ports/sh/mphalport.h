
#include <gint/clock.h>
#include <time.h>

void mp_hal_set_interrupt_char(char c);

static inline void mp_hal_delay_ms(mp_uint_t ms)
{
    sleep_ms(ms);
}

static inline void mp_hal_delay_us(mp_uint_t us)
{
    sleep_us(us);
}

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

static inline uint64_t mp_hal_time_ns(void)
{
    return (uint64_t)time(NULL) * 1000000000;
}
