/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/frozenmod.h"
#include "py/mphal.h"
#if MICROPY_HW_ENABLE_USB
#include "irq.h"
#include "usb.h"
#endif
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "genhdr/mpversion.h"

int pyexec_system_exit = 0;

#if MICROPY_REPL_INFO
STATIC bool repl_display_debugging_info = 0;
#endif

#define EXEC_FLAG_PRINT_EOF             (1 << 0)
#define EXEC_FLAG_ALLOW_DEBUGGING       (1 << 1)
#define EXEC_FLAG_IS_REPL               (1 << 2)
#define EXEC_FLAG_SOURCE_IS_RAW_CODE    (1 << 3)
#define EXEC_FLAG_SOURCE_IS_VSTR        (1 << 4)
#define EXEC_FLAG_SOURCE_IS_FILENAME    (1 << 5)
#define EXEC_FLAG_SOURCE_IS_READER      (1 << 6)
#define EXEC_FLAG_SOURCE_IS_STR         (1 << 7)
#define EXEC_FLAG_NO_INTERRUPT          (1 << 8)

// parses, compiles and executes the code in the lexer
// frees the lexer before returning
// EXEC_FLAG_PRINT_EOF prints 2 EOF chars: 1 after normal output, 1 after exception output
// EXEC_FLAG_ALLOW_DEBUGGING allows debugging info to be printed after executing the code
// EXEC_FLAG_IS_REPL is used for REPL inputs (flag passed on to mp_compile)
STATIC int parse_compile_execute(const void *source, mp_parse_input_kind_t input_kind, mp_uint_t exec_flags) {
    int ret = 0;
    #if MICROPY_REPL_INFO
    uint32_t start = 0;
    #endif

    #ifdef MICROPY_BOARD_BEFORE_PYTHON_EXEC
    MICROPY_BOARD_BEFORE_PYTHON_EXEC(input_kind, exec_flags);
    #endif

    // by default a SystemExit exception returns 0
    pyexec_system_exit = 0;

    nlr_buf_t nlr;
    nlr.ret_val = NULL;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t module_fun;
        #if MICROPY_MODULE_FROZEN_MPY
        if (exec_flags & EXEC_FLAG_SOURCE_IS_RAW_CODE) {
            // source is a raw_code object, create the function
            const mp_frozen_module_t *frozen = source;
            mp_module_context_t *ctx = m_new_obj(mp_module_context_t);
            ctx->module.globals = mp_globals_get();
            ctx->constants = frozen->constants;
            module_fun = mp_make_function_from_raw_code(frozen->rc, ctx, NULL);
        } else
        #endif
        {
            #if MICROPY_ENABLE_COMPILER
            mp_lexer_t *lex;
            if (exec_flags & EXEC_FLAG_SOURCE_IS_VSTR) {
                const vstr_t *vstr = source;
                lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, vstr->buf, vstr->len, 0);
            } else if (exec_flags & EXEC_FLAG_SOURCE_IS_STR) {
                lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_,
                    source, strlen(source), 0);
            } else if (exec_flags & EXEC_FLAG_SOURCE_IS_READER) {
                lex = mp_lexer_new(MP_QSTR__lt_stdin_gt_, *(mp_reader_t *)source);
            } else if (exec_flags & EXEC_FLAG_SOURCE_IS_FILENAME) {
                lex = mp_lexer_new_from_file(qstr_from_str(source));
            } else {
                lex = (mp_lexer_t *)source;
            }
            // source is a lexer, parse and compile the script
            qstr source_name = lex->source_name;
            mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
            module_fun = mp_compile(&parse_tree, source_name, exec_flags & EXEC_FLAG_IS_REPL);
            #else
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("script compilation not supported"));
            #endif
        }

        // execute code
        if (!(exec_flags & EXEC_FLAG_NO_INTERRUPT)) {
            mp_hal_set_interrupt_char(CHAR_CTRL_C);
        }
        #if MICROPY_REPL_INFO
        start = mp_hal_ticks_ms();
        #endif
        mp_call_function_0(module_fun);
        mp_hal_set_interrupt_char(-1); // disable interrupt
        mp_handle_pending(true); // handle any pending exceptions (and any callbacks)
        nlr_pop();
        ret = 1;
        if (exec_flags & EXEC_FLAG_PRINT_EOF) {
            mp_hal_stdout_tx_strn("\x04", 1);
        }
    } else {
        // uncaught exception
        mp_hal_set_interrupt_char(-1); // disable interrupt
        mp_handle_pending(false); // clear any pending exceptions (and run any callbacks)

        if (exec_flags & EXEC_FLAG_SOURCE_IS_READER) {
            const mp_reader_t *reader = source;
            reader->close(reader->data);
        }

        // print EOF after normal output
        if (exec_flags & EXEC_FLAG_PRINT_EOF) {
            mp_hal_stdout_tx_strn("\x04", 1);
        }

        // check for SystemExit
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t *)nlr.ret_val)->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // at the moment, the value of SystemExit is unused
            ret = pyexec_system_exit;
        } else {
            mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
            ret = 0;
        }
    }

    #if MICROPY_REPL_INFO
    // display debugging info if wanted
    if ((exec_flags & EXEC_FLAG_ALLOW_DEBUGGING) && repl_display_debugging_info) {
        mp_uint_t ticks = mp_hal_ticks_ms() - start; // TODO implement a function that does this properly
        mp_printf(&mp_plat_print, "took " UINT_FMT " ms\n", ticks);
        // qstr info
        {
            size_t n_pool, n_qstr, n_str_data_bytes, n_total_bytes;
            qstr_pool_info(&n_pool, &n_qstr, &n_str_data_bytes, &n_total_bytes);
            mp_printf(&mp_plat_print, "qstr:\n  n_pool=%u\n  n_qstr=%u\n  "
                "n_str_data_bytes=%u\n  n_total_bytes=%u\n",
                (unsigned)n_pool, (unsigned)n_qstr, (unsigned)n_str_data_bytes, (unsigned)n_total_bytes);
        }

        #if MICROPY_ENABLE_GC
        // run collection and print GC info
        gc_collect();
        gc_dump_info(&mp_plat_print);
        #endif
    }
    #endif

    if (exec_flags & EXEC_FLAG_PRINT_EOF) {
        mp_hal_stdout_tx_strn("\x04", 1);
    }

    #ifdef MICROPY_BOARD_AFTER_PYTHON_EXEC
    MICROPY_BOARD_AFTER_PYTHON_EXEC(input_kind, exec_flags, nlr.ret_val, &ret);
    #endif

    return ret;
}

#if MICROPY_ENABLE_COMPILER

void pyexec_event_repl_init(void) {
    MP_STATE_VM(repl_line) = vstr_new(32);

    #ifdef FX9860G
    mp_hal_stdout_tx_str("MicroPython " MICROPY_GIT_TAG "\n");
    #else
    mp_hal_stdout_tx_str(MICROPY_BANNER_NAME_AND_VERSION);
    mp_hal_stdout_tx_str("; " MICROPY_BANNER_MACHINE);
    mp_hal_stdout_tx_str("\n");
    #endif

    #if MICROPY_PY_BUILTINS_HELP
    #ifdef FX9860G
    mp_hal_stdout_tx_str("Type \"help()\" for info.\n");
    #else
    mp_hal_stdout_tx_str("Type \"help()\" for more information.\n");
    #endif
    #endif
    vstr_reset(MP_STATE_VM(repl_line));
}

STATIC int pyexec_raw_repl_process_char(int c) {
    if(c != CHAR_CTRL_D && c != '\r') {
        vstr_add_byte(MP_STATE_VM(repl_line), c);
        return 0;
    }

    int ret = parse_compile_execute(MP_STATE_VM(repl_line),
        MP_PARSE_SINGLE_INPUT,
        EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL |
            EXEC_FLAG_SOURCE_IS_VSTR);
    if (ret & PYEXEC_FORCED_EXIT) {
        return ret;
    }

    vstr_reset(MP_STATE_VM(repl_line));
    return 0;
}

uint8_t pyexec_repl_active;
int pyexec_event_repl_process_char(int c) {
    pyexec_repl_active = 1;
    int res = pyexec_raw_repl_process_char(c);
    pyexec_repl_active = 0;
    return res;
}

int pyexec_repl_execute(char const *line) {
    pyexec_repl_active = 1;

    int ret = parse_compile_execute(line,
        MP_PARSE_SINGLE_INPUT,
        EXEC_FLAG_ALLOW_DEBUGGING | EXEC_FLAG_IS_REPL |
            EXEC_FLAG_SOURCE_IS_STR);
    if (ret & PYEXEC_FORCED_EXIT) {
        return ret;
    }

    pyexec_repl_active = 0;
    return 0;
}

MP_REGISTER_ROOT_POINTER(vstr_t * repl_line);

#endif // MICROPY_ENABLE_COMPILER

int pyexec_file(const char *filename) {
    return parse_compile_execute(filename, MP_PARSE_FILE_INPUT, EXEC_FLAG_SOURCE_IS_FILENAME);
}

int pyexec_file_if_exists(const char *filename) {
    #if MICROPY_MODULE_FROZEN
    if (mp_find_frozen_module(filename, NULL, NULL) == MP_IMPORT_STAT_FILE) {
        return pyexec_frozen_module(filename, true);
    }
    #endif
    if (mp_import_stat(filename) != MP_IMPORT_STAT_FILE) {
        return 1; // success (no file is the same as an empty file executing without fail)
    }
    return pyexec_file(filename);
}

#if MICROPY_MODULE_FROZEN
int pyexec_frozen_module(const char *name, bool allow_keyboard_interrupt) {
    void *frozen_data;
    int frozen_type;
    mp_find_frozen_module(name, &frozen_type, &frozen_data);
    mp_uint_t exec_flags = allow_keyboard_interrupt ? 0 : EXEC_FLAG_NO_INTERRUPT;

    switch (frozen_type) {
        #if MICROPY_MODULE_FROZEN_STR
        case MP_FROZEN_STR:
            return parse_compile_execute(frozen_data, MP_PARSE_FILE_INPUT, exec_flags);
        #endif

        #if MICROPY_MODULE_FROZEN_MPY
        case MP_FROZEN_MPY:
            return parse_compile_execute(frozen_data, MP_PARSE_FILE_INPUT, exec_flags |
                EXEC_FLAG_SOURCE_IS_RAW_CODE);
        #endif

        default:
            mp_printf(MICROPY_ERROR_PRINTER, "could not find module '%s'\n", name);
            return false;
    }
}
#endif

#if MICROPY_REPL_INFO
mp_obj_t pyb_set_repl_info(mp_obj_t o_value) {
    repl_display_debugging_info = mp_obj_get_int(o_value);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_set_repl_info_obj, pyb_set_repl_info);
#endif
