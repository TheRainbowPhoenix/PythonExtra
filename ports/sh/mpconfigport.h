//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.mpconfigport: MicroPython's main port configuration file

#include <stdint.h>
#include <alloca.h>
#include <gint/rtc.h>
#include "widget_shell.h"

/* Debugging options: PythonExtra debug tools (pretty much required for any
   other one), MicroPython's verbose logging. */
/* PythonExtra's main debug flag */
#define PE_DEBUG                          (1)
#define MICROPY_DEBUG_VERBOSE             (0)
/* Custom flag to remove DEBUG_printf in alloc/GC (very verbose) */
#define MICROPY_DEBUG_VERBOSE_ALLOC       (0)

#if PE_DEBUG
extern const struct _mp_print_t mp_debug_print;
#define MICROPY_DEBUG_PRINTER             (&mp_debug_print)
#endif

/* Custom option to use relative imports. For instance when working at the fs
   root, 'import b' in '/folder/a.py' will import 'folder/b.py' not '/b.py'. */
#define MICROPY_RELATIVE_FILE_IMPORTS     (1)

/* General feature set selection
   Other options: BASIC_FEATURES, EXTRA_FEATURES, FULL_FEATURES, EVERYTHING */
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)

/* Main features */
#define MICROPY_ENABLE_COMPILER           (1)
#define MICROPY_ENABLE_GC                 (1)
#define MICROPY_GC_SPLIT_HEAP             (1)
#define MP_ENDIANNESS_BIG                 (1)
#define MICROPY_READER_POSIX              (1)
#define MICROPY_ERROR_REPORTING           (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_LONGINT_IMPL              (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL                (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_REPL_EVENT_DRIVEN         (1)

/* Other features that we select against MICROPY_CONFIG_ROM_LEVEL */
#define MICROPY_PY_FSTRINGS               (1) /* in EXTRA_FEATURES */
#define MICROPY_HELPER_REPL               (1) /* in EXTRA_FEATURES */
#define MICROPY_ENABLE_SOURCE_LINE        (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_BUILTINS_STR_UNICODE   (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_BUILTINS_HELP_MODULES  (1) /* in EXTRA_FEATURES */
#define MICROPY_KBD_EXCEPTION             (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_SYS_PS1_PS2            (1) /* in EXTRA_FEATURES */
#define MICROPY_MODULE_BUILTIN_INIT       (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_ALL_SPECIAL_METHODS    (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_REVERSE_SPECIAL_METHODS (1) /* in EXTRA_FEATURES */
#define MICROPY_PY_BUILTINS_ROUND_INT     (1) /* in EXTRA_FEATURES */
// #define MICROPY_PY_SYS_STDFILES           (1) /* in EXTRA_FEATURES */

#define MICROPY_ALLOC_PATH_MAX            (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT    (32)
#define MICROPY_MEM_STATS                 (0)
#define MICROPY_GC_ALLOC_THRESHOLD        (1)
#define MICROPY_ENABLE_DOC_STRING         (0)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (1)

#define MICROPY_PY_BUILTINS_BYTEARRAY     (1)
#define MICROPY_PY_BUILTINS_ENUMERATE     (1)
#define MICROPY_PY_BUILTINS_FILTER        (1)
#define MICROPY_PY_BUILTINS_FROZENSET     (1)
#define MICROPY_PY_BUILTINS_HELP          (1)
#define MICROPY_PY_BUILTINS_INPUT         (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW    (1)
#define MICROPY_PY_BUILTINS_MIN_MAX       (1)
#define MICROPY_PY_BUILTINS_PROPERTY      (1)
#define MICROPY_PY_BUILTINS_REVERSED      (1)
#define MICROPY_PY_BUILTINS_SET           (1)
#define MICROPY_PY_BUILTINS_SLICE         (1)

/* Extra built-in modules */
#define MICROPY_PY_ARRAY                  (1)
#define MICROPY_PY_COLLECTIONS            (1)
#define MICROPY_PY_MATH                   (1)
#define MICROPY_PY_CMATH                  (1)
#define MICROPY_PY_GC                     (1)
#define MICROPY_PY_IO                     (1)
#define MICROPY_PY_STRUCT                 (1)
#define MICROPY_PY_RANDOM                 (1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS     (1)
#define MICROPY_PY_SYS                    (1)
#define MICROPY_PY_TIME                   (1)
#define MICROPY_PY_TIME_TIME_TIME_NS      (1)
#define MICROPY_PY_TIME_INCLUDEFILE       "ports/sh/modtime.c"
// TODO: Enable the os module:
// #define MICROPY_PY_UOS                    (1)
// TODO: Enable other modules
// #define MICROPY_PY_URE                    (1) // + other flags?

/* Enable alias of u-modules */
#define MICROPY_MODULE_WEAK_LINKS         (1)

/* Seed for random module */
#define MICROPY_PY_RANDOM_SEED_INIT_FUNC (rtc_ticks())

/* Command executed automatically after every shell input */
void pe_after_python_exec(
    int input_kind, int exec_flags, void *ret_val, int *ret);
#define MICROPY_BOARD_AFTER_PYTHON_EXEC pe_after_python_exec

/* Command executed regularly during execution */
extern void pe_draw(void);
extern widget_shell *pe_shell;
extern void pe_dupdate(void);
extern bool pe_dupdate_scheduled;
#define MICROPY_VM_HOOK_LOOP \
    { if(pe_shell->widget.update) pe_draw(); \
      if(pe_dupdate_scheduled) pe_dupdate(); }

/* extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) }, */

/* Machine-specific type definitions */
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long mp_off_t;

#define MICROPY_HW_BOARD_NAME "sh7305"
#define MICROPY_HW_MCU_NAME   "sh-4a"

#define MP_STATE_PORT MP_STATE_VM

