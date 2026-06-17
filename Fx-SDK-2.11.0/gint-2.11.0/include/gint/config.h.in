//---
//	config - Compile-time generate configuration
//---

#ifndef GINT_CONFIG
#define GINT_CONFIG

/* GINT_VERSION: Latest tag and number of additional commits
     "2.1.0"   = Release 2.1.0
     "2.1.1-5" = 5 commits after release 2.1.1 */
#define GINT_VERSION "@GINT_GIT_VERSION@"

/* GINT_HASH: Commit hash with 7 digits
     0x03f7c0a0 = Commit 3f7c0a0 */
#define GINT_HASH 0x@GINT_GIT_HASH@

/* GINT_HW_{FX,CG}: Identifies the type of hardware running the program. */
#if defined(FX9860G)
# define GINT_HW_FX 1
# define GINT_HW_CG 0
# define GINT_HW_CP 0
# define GINT_HW_SWITCH(FX,CG,CP) (FX)
#elif defined(FXCG50)
# define GINT_HW_FX 0
# define GINT_HW_CG 1
# define GINT_HW_CP 0
# define GINT_HW_SWITCH(FX,CG,CP) (CG)
#elif defined(FXCP)
# define GINT_HW_FX 0
# define GINT_HW_CG 0
# define GINT_HW_CP 1
# define GINT_HW_SWITCH(FX,CG,CP) (CP)
#endif

/* Shorthand to simplify definitions below. Won't be needed for long. */
#if defined(FX9860G_G3A)
# define GINT_FX9860G_G3A 1
#else
# define GINT_FX9860G_G3A 0
#endif

/* GINT_OS_{FX,CG}: Identifies the type of OS API we're assuming. Currently I
   see no reason this would be different from hardware, but who knows. */
#define GINT_OS_FX GINT_HW_FX
#define GINT_OS_CG GINT_HW_CG
#define GINT_OS_CP GINT_HW_CP
#define GINT_OS_SWITCH GINT_HW_SWITCH

/* GINT_NO_OS_STACK: Disables using a chunk of the OS stack as a heap. The top
   section covering 355/512 ko is otherwise used. (fx-CG 50) */
#cmakedefine GINT_NO_OS_STACK

/* GINT_USER_VRAM: Selects whether to store VRAMs in the user stack or in the
   OS stack. Deprecated, now controlled by GINT_NO_OS_STACK. (fx-CG 50) */
#cmakedefine GINT_USER_VRAM

#ifdef GINT_USER_VRAM
# define GINT_NO_OS_STACK
#endif

/* GINT_STATIC_GRAY: Selects whether additional gray VRAMs are allocated
   statically or in the system heap (fx-9860G) */
#cmakedefine GINT_STATIC_GRAY

/* GINT_KMALLOC_DEBUG: Selects whether kmalloc debug functions are enabled
   (these are mainly data structure integrity checks and information that make
   sense for a developer). This is independent from statistics, which can be
   enabled or disabled at runtime. */
#cmakedefine GINT_KMALLOC_DEBUG

/* GINT_USB_DEBUG: Selects whether USB debug functions are enabled */
#cmakedefine GINT_USB_DEBUG

/* GINT_RENDER_DMODE: Selects whether the dmode override is available on
   rendering functions. */
#define GINT_RENDER_DMODE (GINT_HW_FX || GINT_FX9860G_G3A)

/* GINT_RENDER_{MONO,RGB}: Enable the mono/rgb rendering API.
   Currently these are exclusive. */
#define GINT_RENDER_MONO (GINT_HW_FX || GINT_FX9860G_G3A)
#define GINT_RENDER_RGB ((GINT_HW_CG || GINT_HW_CP) && !GINT_FX9860G_G3A)

#endif /* GINT_CONFIG */
