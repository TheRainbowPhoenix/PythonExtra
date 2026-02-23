//---
// gint:image:fixed - Minimal fixed-point interface for linear transformations
//---

#ifndef GINT_IMAGE_FIXED
#define GINT_IMAGE_FIXED

#include <stdint.h>

/* Constants */
#define fconst(x) ((x) * 65536)

/* Multiplication */
static inline int fmul(int x, int y)
{
    return ((int64_t)x * (int64_t)y) >> 16;
}

/* Division */
static inline int fdiv(int x, int y)
{
    return ((int64_t)x << 16) / y;
}

/* Integer square root */
static inline int isqrt(int n)
{
    if(n <= 0) return 0;
    if(n < 4) return 1;

    int low_bound = isqrt(n / 4) * 2;
    int high_bound = low_bound + 1;

    return (high_bound * high_bound <= n) ? high_bound : low_bound;
}

/* Floor operation */
static inline int ffloor(int x)
{
    return (x >> 16);
}

/* Round operation */
static inline int fround(int x)
{
    return ffloor(x + fconst(0.5));
}

#endif /* GINT_IMAGE_FIXED */
