//---
//	gint:hardware - Platform information and hardware detection
//
//	This components centralizes detected information about the runtime
//	hardware, including MPU version, peripheral modules, and how drivers
//	configured them.
//
//	The most common use of this header is for the isSH3() and isSH4()
//	macros that let you run MPU-dependent jobs and are used like this:
//	  if(isSH3()) do_sh3();
//	  else do_sh4();
//---

#ifndef GINT_HARDWARE
#define GINT_HARDWARE

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>

/* For compatibility with ASM, include the following bits only in C code */
#ifndef CPP_ASM

#include <gint/defs/types.h>

/* Most of the information here is going to be stored in (key, value) pairs for
   predetermined keys and 32-bits values that are often integers or a set of
   flags. The data will be filled by gint or its drivers. */
#define HW_KEYS 16
extern uint32_t gint[HW_KEYS];

/* hw_detect(): Basic hardware detection
   This function probes the hardware and fills in the HWMPU, HWCPUVR and
   HWCPUPR fields. */
void hw_detect(void);

#endif /* CPP_ASM */

/* MPU detection macros, with a faster version on fx-CG 50 and a generic
   dual-platform version for libraries.
   Warning: this macro is also used hardcoded in exch.s. */

#if GINT_HW_FX
# define isSH3() (gint[HWMPU] & 1)
# define isSH4() (!isSH3())
# define isSlim() (gint[HWCALC] == HWCALC_FX9860G_SLIM)
#elif GINT_HW_CG || GINT_HW_CP
# define isSH3() 0
# define isSH4() 1
# define isSlim() 0
#endif

/* This bit should be set in all data longwords except HWMPU, HWCPUVR, HWCPUPR
   and HWCALC which are guaranteed to always be loaded. If not set then the
   information must be treated as invalid. */
#define HW_LOADED 0x80000000

/*
**  Key list
*/

#define HWMPU    0    /* MPU type */
#define HWCPUVR  1    /* CPU Version Register */
#define HWCPUPR  2    /* CPU Product Register */
#define HWCALC   3    /* Calculator model, hardcoded in kernel/inth.S */
#define HWRAM    4    /* Amount of RAM */
#define HWROM    5    /* Amount of ROM */
#define HWURAM   6    /* Userspace RAM */
#define HWETMU        /* Deprecated: use timer_count() (ETMU always load) */
#define HWKBD    8    /* Keyboard */
#define HWKBDSF       /* Deprecated: use keysc_scan_frequency() */
#define HWDD          /* Deprecated: use the T6K11/R61524 API */
#define HWFS    11    /* Filesystem type */

/*
**  MPU type
*/

/* Unknown MPUs are all assumed to be SH-4A-based */
#define HWMPU_UNKNOWN  0
/* Used on original fx-9860G, SH-3-based */
#define HWMPU_SH7337   1
/* Used on recent fx-9860G derivatives such as the fx-9750G II, and also on the
   fx-CG 10/20/50. SH-4A-based */
#define HWMPU_SH7305   2
/* Used on the fx-9860G II, SH-3-based */
#define HWMPU_SH7355   3
/* Closest documented match to the SH7305, not used in any known calculator.
   Detected and included for reference only */
#define HWMPU_SH7724   4

/*
**  Calculator type
*/

/* SH-3-based fx-9860G-family */
#define HWCALC_FX9860G_SH3   1
/* Other SH-4A-based fx-9860G-family */
#define HWCALC_FX9860G_SH4   2
/* Graph 35+E II, an SH-4A French extension of the fx-9860G family */
#define HWCALC_G35PE2        3
/* fx-CG 10/20, also known as the "Prizm" family */
#define HWCALC_PRIZM         4
/* fx-CG 50, a late extension to the Prizm family */
#define HWCALC_FXCG50        5
/* fx-CG 50 emulator, hardcoded in kernel/inth.S */
#define HWCALC_FXCG_MANAGER  6
/* fx-9860G Slim, SH-3-based fx-9860G with hardware differences */
#define HWCALC_FX9860G_SLIM  7
/* fx-CP 400 */
#define HWCALC_FXCP400       8

/*
**  Keyboard
*/

/* The keyboard uses an I/O-port-based scan method. This is possible on both
   SH3 and SH4, but gint will normally do it only on SH3. */
#define HWKBD_IO       0x01
/* When using the I/O-port scanning method on SH3, whether the watchdog is used
   to delay I/O operations. */
#define HWKBD_WDD      0x02
/* The keyboard uses a KEYSC-based scan method. This is only possible on SH4 */
#define HWKBD_KSI      0x04

/*
**  Filesystem type
*/

/* Unknown or no filesystem. */
#define HWFS_NONE        0
/* CASIO's in-house filesystem, now deprecated. */
#define HWFS_CASIOWIN    1
/* Wrapper around Kyoto Software Research's Fugue VFAT implementation. */
#define HWFS_FUGUE       2

#ifdef __cplusplus
}
#endif

#endif /* GINT_HARDWARE */
