//---
//	gint:defs:util - Various utility macros
//---

#ifndef GINT_DEFS_UTIL
#define GINT_DEFS_UTIL

/* synco instruction (in a form compatible with sh3eb-elf) */
#define synco() __asm__ volatile (".word 0x00ab":::"memory")

#ifdef __cplusplus
#define GAUTOTYPE auto
#else
#define GAUTOTYPE __auto_type
#endif

#ifdef __cplusplus
#include <algorithm>
using std::min;
using std::max;
#else
/* min(), max() (without double evaluation) */
#define	min(a, b) ({			\
	GAUTOTYPE _a = (a);		\
	GAUTOTYPE _b = (b);		\
	_a < _b ? _a : _b;		\
})
#define	max(a, b) ({			\
	GAUTOTYPE _a = (a);		\
	GAUTOTYPE _b = (b);		\
	_a > _b ? _a : _b;		\
})
#endif

/* sgn() (without double evaluation) */
#define	sgn(s) ({			\
	GAUTOTYPE _s = (s);		\
	_s < 0 ? -1 :			\
	_s > 0 ? +1 :			\
	0;				\
})

/* swap() - exchange two variables of the same type */
#define	swap(a, b) ({			\
	GAUTOTYPE _tmp = (a);		\
	(a) = (b);			\
	(b) = _tmp;			\
})

#endif /* GINT_DEFS_UTIL */
