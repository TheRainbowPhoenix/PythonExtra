// SH specific stuff

#include "py/mpconfig.h"

#if MICROPY_EMIT_SH

// This is defined so that the assembler exports generic assembler API macros
#define GENERIC_ASM_API (1)
#include "py/asmsh.h"

// Word indices of REG_LOCAL_x in nlr_buf_t
// We assume setjmp saves R8 at the beginning of jmp_buf.
// nlr_buf_t: [prev, ret_val, jmp_buf...]
// So index 2.
#define NLR_BUF_IDX_LOCAL_1 (2) // r8

#define N_SH (1)
#define EXPORT_FUN(name) emit_native_sh_##name
#include "py/emitnative.c"

#endif
