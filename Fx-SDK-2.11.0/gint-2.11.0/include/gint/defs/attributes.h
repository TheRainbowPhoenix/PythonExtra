//---
//	gint:defs:attributes - Macros for compiler-specific attributes
//---

#ifndef GINT_DEFS_ATTRIBUTES
#define GINT_DEFS_ATTRIBUTES

/* Objects from specific sections */
#define GSECTION(x)	__attribute__((section(x)))
/* Objects from the gint's uninitialized BSS section */
#define GBSS		__attribute__((section(".gint.bss")))
/* Additional sections that are only needed on SH3 */
#define GRODATA3	__attribute__((section(".gint.rodata.sh3")))
#define GDATA3		__attribute__((section(".gint.data.sh3")))
#define GBSS3		__attribute__((section(".gint.bss.sh3")))
/* Objects for the ILRAM, XRAM and YRAM regions */
#define GILRAM		__attribute__((section(".ilram")))
#define GXRAM		__attribute__((section(".xyram")))
#define GYRAM		__attribute__((section(".xyram")))

/* Unused parameters or variables */
#define GUNUSED		__attribute__((unused))
/* Functions that *must* be inlined */
#define GINLINE		__attribute__((always_inline)) inline

/* Aligned variables */
#define GALIGNED(x)	__attribute__((aligned(x)))
/* Packed structures. I require explicit alignment because if it's unspecified,
   GCC cannot optimize access size, and reads to memory-mapped I/O with invalid
   access sizes silently fail - honestly you don't want this to happen */
#define GPACKED(x)	__attribute__((packed, aligned(x)))
/* Packed enumerations */
#define GPACKEDENUM	__attribute__((packed))
/* Transparent unions */
#define GTRANSPARENT	__attribute__((transparent_union))

/* Functions and globals that are visible through whole-program optimization */
#define GVISIBLE  __attribute__((externally_visible))

/* Weak symbols */
#define GWEAK		__attribute__((weak))

/* Constructors */
#define GCONSTRUCTOR	__attribute__((constructor))
#define GDESTRUCTOR	__attribute__((destructor))

/* Functions that do not return */
#define GNORETURN	__attribute__((noreturn))

#endif /* GINT_DEFS_ATTRIBUTES */
