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
#ifndef MICROPY_INCLUDED_PY_ASMSH_H
#define MICROPY_INCLUDED_PY_ASMSH_H

#include "py/misc.h"
#include "py/asmbase.h"

#define ASM_SH_REG_R0  (0)
#define ASM_SH_REG_R1  (1)
#define ASM_SH_REG_R2  (2)
#define ASM_SH_REG_R3  (3)
#define ASM_SH_REG_R4  (4)
#define ASM_SH_REG_R5  (5)
#define ASM_SH_REG_R6  (6)
#define ASM_SH_REG_R7  (7)
#define ASM_SH_REG_R8  (8)
#define ASM_SH_REG_R9  (9)
#define ASM_SH_REG_R10 (10)
#define ASM_SH_REG_R11 (11)
#define ASM_SH_REG_R12 (12)
#define ASM_SH_REG_R13 (13)
#define ASM_SH_REG_R14 (14)
#define ASM_SH_REG_R15 (15)

// SH4 specific registers
#define ASM_SH_REG_PC  (16)
#define ASM_SH_REG_PR  (17)
#define ASM_SH_REG_SR  (18)
#define ASM_SH_REG_GBR (19)
#define ASM_SH_REG_VBR (20)
#define ASM_SH_REG_MACH (21)
#define ASM_SH_REG_MACL (22)

typedef struct _asm_sh_t {
    mp_asm_base_t base;
    uint32_t push_reglist;
    uint32_t stack_adjust;
} asm_sh_t;

static inline void asm_sh_end_pass(asm_sh_t *as) {
    (void)as;
}

void asm_sh_entry(asm_sh_t *as, int num_locals);
void asm_sh_exit(asm_sh_t *as);

// mov
void asm_sh_mov_reg_reg(asm_sh_t *as, uint reg_dest, uint reg_src);
void asm_sh_mov_reg_i32(asm_sh_t *as, uint rd, int imm);
void asm_sh_mov_local_reg(asm_sh_t *as, int local_num, uint rd);
void asm_sh_mov_reg_local(asm_sh_t *as, uint rd, int local_num);
void asm_sh_mov_reg_local_addr(asm_sh_t *as, uint rd, int local_num);
void asm_sh_mov_reg_pcrel(asm_sh_t *as, uint reg_dest, uint label);

// arithmetic
void asm_sh_add_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd += rm
void asm_sh_sub_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd -= rm
void asm_sh_mul_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd *= rm (signed 32-bit)
void asm_sh_and_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd &= rm
void asm_sh_or_reg_reg(asm_sh_t *as, uint rd, uint rm);  // rd |= rm
void asm_sh_xor_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd ^= rm
void asm_sh_not_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd = ~rm
void asm_sh_neg_reg_reg(asm_sh_t *as, uint rd, uint rm); // rd = -rm
void asm_sh_lsl_reg_reg(asm_sh_t *as, uint rd, uint rs); // rd <<= rs
void asm_sh_lsr_reg_reg(asm_sh_t *as, uint rd, uint rs); // rd >>= rs (unsigned)
void asm_sh_asr_reg_reg(asm_sh_t *as, uint rd, uint rs); // rd >>= rs (signed)

// comparisons
void asm_sh_cmp_reg_reg(asm_sh_t *as, uint rd, uint rm); // T = (rd == rm) or similar
void asm_sh_cmp_reg_i8(asm_sh_t *as, uint rd, int imm); // T = (rd == imm)
void asm_sh_compare_op(asm_sh_t *as, uint op_idx, uint rn, uint rm);

// conditional set
// SH4 uses T-bit. To set a register based on T-bit requires explicit code.
// cond here interprets op_idx (0-11) for comparison results.
void asm_sh_setcc_reg(asm_sh_t *as, uint rd, uint cond);

// memory load/store
void asm_sh_ldr_reg_reg(asm_sh_t *as, uint rd, uint rm, uint byte_offset);
void asm_sh_ldrb_reg_reg(asm_sh_t *as, uint rd, uint rm);
void asm_sh_ldrh_reg_reg(asm_sh_t *as, uint rd, uint rm);
void asm_sh_ldrh_reg_reg_offset(asm_sh_t *as, uint rd, uint rm, uint byte_offset);
void asm_sh_str_reg_reg(asm_sh_t *as, uint rd, uint rm, uint byte_offset);
void asm_sh_strb_reg_reg(asm_sh_t *as, uint rd, uint rm);
void asm_sh_strh_reg_reg(asm_sh_t *as, uint rd, uint rm);

// memory load/store indexed
void asm_sh_str_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn); // *(rm+rn) = rd (32-bit)
void asm_sh_strh_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn);
void asm_sh_strb_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn);

// control flow
void asm_sh_b_label(asm_sh_t *as, uint label);
void asm_sh_bcc_label(asm_sh_t *as, int cond, uint label);
void asm_sh_bl_ind(asm_sh_t *as, uint fun_id, uint reg_temp);
void asm_sh_bx_reg(asm_sh_t *as, uint reg_src);

// SH4 specific conditions
#define ASM_SH_CC_EQ (0) // T=1
#define ASM_SH_CC_NE (1) // T=0
#define ASM_SH_CC_HS (2) // T=1 (unsigned >=)
#define ASM_SH_CC_LO (3) // T=1 (unsigned <) - wait, check this.
#define ASM_SH_CC_HI (4) // T=1 (unsigned >)
#define ASM_SH_CC_LS (5) // T=1 (unsigned <=)
#define ASM_SH_CC_GE (6) // T=1 (signed >=)
#define ASM_SH_CC_LT (7) // T=1 (signed <)
#define ASM_SH_CC_GT (8) // T=1 (signed >)
#define ASM_SH_CC_LE (9) // T=1 (signed <=)

// Register aliases
#define ASM_WORD_SIZE (4)

#define REG_RET ASM_SH_REG_R0
#define REG_ARG_1 ASM_SH_REG_R4
#define REG_ARG_2 ASM_SH_REG_R5
#define REG_ARG_3 ASM_SH_REG_R6
#define REG_ARG_4 ASM_SH_REG_R7

#define REG_TEMP0 ASM_SH_REG_R1
#define REG_TEMP1 ASM_SH_REG_R2
#define REG_TEMP2 ASM_SH_REG_R3

#define REG_LOCAL_1 ASM_SH_REG_R8
#define REG_LOCAL_2 ASM_SH_REG_R9
#define REG_LOCAL_3 ASM_SH_REG_R10
#define REG_LOCAL_NUM (3)

// Holds a pointer to mp_fun_table
// SH4 uses R12 (conventionally global pointer, but R11/R13 are saved regs)
// Let's use R11 as it is callee-saved, or maybe we can spare R12 if we save it.
// Wait, generic ASM uses REG_FUN_TABLE.
// Let's use R12 for FUN_TABLE, but we must save/restore it if it's callee-saved or check calling convention.
// On SH4, R8-R15 are callee-saved.
#define REG_FUN_TABLE ASM_SH_REG_R11

#define ASM_T               asm_sh_t
#define ASM_END_PASS        asm_sh_end_pass
#define ASM_ENTRY           asm_sh_entry
#define ASM_EXIT            asm_sh_exit

#define ASM_JUMP            asm_sh_b_label
#define ASM_JUMP_IF_REG_ZERO(as, reg, label, bool_test) \
    do { \
        asm_sh_cmp_reg_i8(as, reg, 0); \
        asm_sh_bcc_label(as, ASM_SH_CC_EQ, label); \
    } while (0)

#define ASM_JUMP_IF_REG_NONZERO(as, reg, label, bool_test) \
    do { \
        asm_sh_cmp_reg_i8(as, reg, 0); \
        asm_sh_bcc_label(as, ASM_SH_CC_NE, label); \
    } while (0)

#define ASM_JUMP_IF_REG_EQ(as, reg1, reg2, label) \
    do { \
        asm_sh_cmp_reg_reg(as, reg1, reg2); \
        asm_sh_bcc_label(as, ASM_SH_CC_EQ, label); \
    } while (0)

#define ASM_JUMP_REG(as, reg) asm_sh_bx_reg((as), (reg))
#define ASM_CALL_IND(as, idx) asm_sh_bl_ind(as, idx, ASM_SH_REG_R3)

#define ASM_MOV_LOCAL_REG(as, local_num, reg_src) asm_sh_mov_local_reg((as), (local_num), (reg_src))
#define ASM_MOV_REG_IMM(as, reg_dest, imm) asm_sh_mov_reg_i32((as), (reg_dest), (imm))
#define ASM_MOV_REG_LOCAL(as, reg_dest, local_num) asm_sh_mov_reg_local((as), (reg_dest), (local_num))
#define ASM_MOV_REG_REG(as, reg_dest, reg_src) asm_sh_mov_reg_reg((as), (reg_dest), (reg_src))
#define ASM_MOV_REG_LOCAL_ADDR(as, reg_dest, local_num) asm_sh_mov_reg_local_addr((as), (reg_dest), (local_num))
#define ASM_MOV_REG_PCREL(as, reg_dest, label) asm_sh_mov_reg_pcrel((as), (reg_dest), (label))

#define ASM_NOT_REG(as, reg_dest) asm_sh_not_reg_reg((as), (reg_dest), (reg_dest))
#define ASM_NEG_REG(as, reg_dest) asm_sh_neg_reg_reg((as), (reg_dest), (reg_dest))
#define ASM_LSL_REG_REG(as, reg_dest, reg_shift) asm_sh_lsl_reg_reg((as), (reg_dest), (reg_shift))
#define ASM_LSR_REG_REG(as, reg_dest, reg_shift) asm_sh_lsr_reg_reg((as), (reg_dest), (reg_shift))
#define ASM_ASR_REG_REG(as, reg_dest, reg_shift) asm_sh_asr_reg_reg((as), (reg_dest), (reg_shift))
#define ASM_OR_REG_REG(as, reg_dest, reg_src) asm_sh_or_reg_reg((as), (reg_dest), (reg_src))
#define ASM_XOR_REG_REG(as, reg_dest, reg_src) asm_sh_xor_reg_reg((as), (reg_dest), (reg_src))
#define ASM_AND_REG_REG(as, reg_dest, reg_src) asm_sh_and_reg_reg((as), (reg_dest), (reg_src))
#define ASM_ADD_REG_REG(as, reg_dest, reg_src) asm_sh_add_reg_reg((as), (reg_dest), (reg_src))
#define ASM_SUB_REG_REG(as, reg_dest, reg_src) asm_sh_sub_reg_reg((as), (reg_dest), (reg_src))
#define ASM_MUL_REG_REG(as, reg_dest, reg_src) asm_sh_mul_reg_reg((as), (reg_dest), (reg_src))

#define ASM_LOAD_REG_REG(as, reg_dest, reg_base) asm_sh_ldr_reg_reg((as), (reg_dest), (reg_base), 0)
#define ASM_LOAD_REG_REG_OFFSET(as, reg_dest, reg_base, word_offset) asm_sh_ldr_reg_reg((as), (reg_dest), (reg_base), 4 * (word_offset))
#define ASM_LOAD8_REG_REG(as, reg_dest, reg_base) asm_sh_ldrb_reg_reg((as), (reg_dest), (reg_base))
#define ASM_LOAD16_REG_REG(as, reg_dest, reg_base) asm_sh_ldrh_reg_reg((as), (reg_dest), (reg_base))
#define ASM_LOAD16_REG_REG_OFFSET(as, reg_dest, reg_base, uint16_offset) asm_sh_ldrh_reg_reg_offset((as), (reg_dest), (reg_base), 2 * (uint16_offset))
#define ASM_LOAD32_REG_REG(as, reg_dest, reg_base) asm_sh_ldr_reg_reg((as), (reg_dest), (reg_base), 0)

#define ASM_STORE_REG_REG(as, reg_value, reg_base) asm_sh_str_reg_reg((as), (reg_value), (reg_base), 0)
#define ASM_STORE_REG_REG_OFFSET(as, reg_dest, reg_base, word_offset) asm_sh_str_reg_reg((as), (reg_dest), (reg_base), 4 * (word_offset))
#define ASM_STORE8_REG_REG(as, reg_value, reg_base) asm_sh_strb_reg_reg((as), (reg_value), (reg_base))
#define ASM_STORE16_REG_REG(as, reg_value, reg_base) asm_sh_strh_reg_reg((as), (reg_value), (reg_base))
#define ASM_STORE32_REG_REG(as, reg_value, reg_base) asm_sh_str_reg_reg((as), (reg_value), (reg_base), 0)

#endif // MICROPY_INCLUDED_PY_ASMSH_H
