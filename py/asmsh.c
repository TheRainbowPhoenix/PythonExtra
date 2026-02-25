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

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "py/mpconfig.h"

#if MICROPY_EMIT_SH

#include "py/asmsh.h"

// Check if an immediate fits in a signed 8-bit field
#define FIT_S8(x) (((x) >= -128) && ((x) <= 127))
#define FIT_S12(x) (((x) >= -2048) && ((x) <= 2047))

static void asm_sh_write_word16(asm_sh_t *as, uint16_t val) {
    mp_asm_base_data(&as->base, 2, val);
}

// Entry/Exit
void asm_sh_entry(asm_sh_t *as, int num_locals) {
    // Callee-saved registers: R8-R13, R14 (FP), R15 (SP), PR.
    // We must save PR and R14, and R8-R13 if used.
    // For simplicity, we save all R8-R13 to be safe and simple.

    // Save PR: STS.L PR, @-R15 (4F 22)
    asm_sh_write_word16(as, 0x4F22);
    // Save R14: MOV.L R14, @-R15 (2F E6)
    asm_sh_write_word16(as, 0x2FE6);
    // Save R13: MOV.L R13, @-R15 (2F D6)
    asm_sh_write_word16(as, 0x2FD6);
    // Save R12: MOV.L R12, @-R15 (2F C6)
    asm_sh_write_word16(as, 0x2FC6);
    // Save R11: MOV.L R11, @-R15 (2F B6)
    asm_sh_write_word16(as, 0x2FB6);
    // Save R10: MOV.L R10, @-R15 (2F A6)
    asm_sh_write_word16(as, 0x2FA6);
    // Save R9: MOV.L R9, @-R15 (2F 96)
    asm_sh_write_word16(as, 0x2F96);
    // Save R8: MOV.L R8, @-R15 (2F 86)
    asm_sh_write_word16(as, 0x2F86);

    // Setup FP: MOV R15, R14 (6E F3)
    asm_sh_write_word16(as, 0x6EF3);

    // Allocate locals
    if (num_locals > 0) {
        int stack_size = num_locals * 4;
        if (stack_size <= 128) {
            // ADD #-stack_size, R15 (7F ii)
            asm_sh_write_word16(as, 0x7F00 | ((-stack_size) & 0xFF));
        } else {
            // Load constant and sub
            asm_sh_mov_reg_i32(as, ASM_SH_REG_R1, stack_size);
            asm_sh_sub_reg_reg(as, ASM_SH_REG_R15, ASM_SH_REG_R1);
        }
    }
}

void asm_sh_exit(asm_sh_t *as) {
    // Restore SP: MOV R14, R15 (6F E3)
    asm_sh_write_word16(as, 0x6FE3);

    // Restore R8-R13
    // MOV.L @R15+, R8 (68 F6)
    asm_sh_write_word16(as, 0x68F6);
    // MOV.L @R15+, R9 (69 F6)
    asm_sh_write_word16(as, 0x69F6);
    // MOV.L @R15+, R10 (6A F6)
    asm_sh_write_word16(as, 0x6AF6);
    // MOV.L @R15+, R11 (6B F6)
    asm_sh_write_word16(as, 0x6BF6);
    // MOV.L @R15+, R12 (6C F6)
    asm_sh_write_word16(as, 0x6CF6);
    // MOV.L @R15+, R13 (6D F6)
    asm_sh_write_word16(as, 0x6DF6);

    // Restore R14: MOV.L @R15+, R14 (6E F6)
    asm_sh_write_word16(as, 0x6EF6);
    // Restore PR: LDS.L @R15+, PR (4F 26)
    asm_sh_write_word16(as, 0x4F26);

    // RTS (00 0B)
    asm_sh_write_word16(as, 0x000B);
    // NOP (delay slot) (00 09)
    asm_sh_write_word16(as, 0x0009);
}

// MOV Rm, Rn (6n m3)
void asm_sh_mov_reg_reg(asm_sh_t *as, uint reg_dest, uint reg_src) {
    asm_sh_write_word16(as, 0x6003 | (reg_dest << 8) | (reg_src << 4));
}

// MOV #imm, Rn (En ii)
void asm_sh_mov_reg_imm8(asm_sh_t *as, uint reg_dest, int imm) {
    assert(FIT_S8(imm));
    asm_sh_write_word16(as, 0xE000 | (reg_dest << 8) | (imm & 0xFF));
}

// Load 32-bit immediate
void asm_sh_mov_reg_i32(asm_sh_t *as, uint rd, int imm) {
    if (FIT_S8(imm)) {
        asm_sh_mov_reg_imm8(as, rd, imm);
    } else {
        // PC-relative load with inline constant

        // Align PC to 4 bytes for constant storage
        uint32_t pos = mp_asm_base_get_code_pos(&as->base);
        if ((pos & 3) != 0) {
            asm_sh_write_word16(as, 0x0009); // NOP
            pos += 2;
        }

        // At 4-byte aligned pos.
        // MOV.L @(0, PC), Rd (D000 + n<<8)
        // Reads from (PC&~3)+4+0 = pos+4+4 = pos+8.
        asm_sh_write_word16(as, 0xD000 | (rd << 8));

        // BRA 1 (A001). Jump over constant.
        // Target = PC + 4 + 1*2 = pos+6 + 4 + 2 = pos+12.
        asm_sh_write_word16(as, 0xA001);

        // NOP (Delay slot)
        asm_sh_write_word16(as, 0x0009);

        // Constant (at pos+6... wait)
        // pos:   MOV.L (2)
        // pos+2: BRA   (2)
        // pos+4: NOP   (2)
        // pos+6: PAD   (2) - Need padding for alignment?

        // Constant must be at pos+8.
        // We have emitted 6 bytes so far (MOV.L, BRA, NOP).
        // Current write pos is pos+6.
        // We need 2 bytes padding to reach pos+8.
        asm_sh_write_word16(as, 0x0009); // NOP (padding)

        // Now at pos+8. 4-byte aligned.
        mp_asm_base_data(&as->base, 4, (uint32_t)imm);

        // Next instruction at pos+12. BRA jumps here. Correct.
    }
}

// Arithmetic
// ADD Rm, Rn (3n mC)
void asm_sh_add_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x300C | (rd << 8) | (rm << 4));
}

// SUB Rm, Rn (3n m8)
void asm_sh_sub_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x3008 | (rd << 8) | (rm << 4));
}

// AND Rm, Rn (2n m9)
void asm_sh_and_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x2009 | (rd << 8) | (rm << 4));
}

// OR Rm, Rn (2n mB)
void asm_sh_or_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x200B | (rd << 8) | (rm << 4));
}

// XOR Rm, Rn (2n mA)
void asm_sh_xor_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x200A | (rd << 8) | (rm << 4));
}

// NOT Rm, Rn (6n m7)
void asm_sh_not_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x6007 | (rd << 8) | (rm << 4));
}

// NEG Rm, Rn (6n mB)
void asm_sh_neg_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x600B | (rd << 8) | (rm << 4));
}

// MUL.L Rm, Rn (0n m7) -> MACL
// STS MACL, Rd (0n 1A)
void asm_sh_mul_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x0007 | (rd << 8) | (rm << 4));
    asm_sh_write_word16(as, 0x001A | (rd << 8));
}

// Shifts
// SHLD Rm, Rn (4n mD). Shift Rn by Rm (signed).
void asm_sh_lsl_reg_reg(asm_sh_t *as, uint rd, uint rs) {
    // SHLD requires shift amount in register.
    // LSL is logical shift left. SHLD does this if amount is positive.
    asm_sh_write_word16(as, 0x400D | (rd << 8) | (rs << 4));
}

// LSR (Logical Shift Right)
// SHLD Rm, Rn. Negative Rm means right shift.
void asm_sh_lsr_reg_reg(asm_sh_t *as, uint rd, uint rs) {
    // Need to negate rs. Use R0 as temp (assumed safe scratch).
    asm_sh_neg_reg_reg(as, ASM_SH_REG_R0, rs);
    asm_sh_write_word16(as, 0x400D | (rd << 8) | (ASM_SH_REG_R0 << 4)); // SHLD R0, Rd
}

// ASR (Arithmetic Shift Right)
// SHAD Rm, Rn (4n mC). Negative Rm means right shift.
void asm_sh_asr_reg_reg(asm_sh_t *as, uint rd, uint rs) {
    asm_sh_neg_reg_reg(as, ASM_SH_REG_R0, rs);
    asm_sh_write_word16(as, 0x400C | (rd << 8) | (ASM_SH_REG_R0 << 4)); // SHAD R0, Rd
}

// Comparisons
// CMP/EQ Rm, Rn (3n m0)
void asm_sh_cmp_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x3000 | (rd << 8) | (rm << 4));
}

// CMP/EQ #imm, R0 (88 ii) - Only R0.
// CMP/EQ #imm, Rn? No direct instruction.
void asm_sh_cmp_reg_i8(asm_sh_t *as, uint rd, int imm) {
    if (rd == ASM_SH_REG_R0 && FIT_S8(imm)) {
        asm_sh_write_word16(as, 0x8800 | (imm & 0xFF));
    } else {
        // Load imm to R1 (TEMP0) then CMP
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R1, imm);
        asm_sh_cmp_reg_reg(as, rd, ASM_SH_REG_R1);
    }
}

// Control Flow
void asm_sh_b_label(asm_sh_t *as, uint label) {
    asm_sh_bcc_label(as, -1, label); // -1 for unconditional
}

void asm_sh_bcc_label(asm_sh_t *as, int cond, uint label) {
    mp_uint_t dest = as->base.label_offsets[label];
    mp_uint_t src = mp_asm_base_get_code_pos(&as->base);
    mp_int_t rel = dest - src - 4; // PC is src + 4

    // rel must be even.
    // disp = rel / 2.
    // Check range. 8-bit or 12-bit?
    // BRA: 12-bit (A0 dd). Range +/- 4KB.
    // BT/BF: 8-bit (89 dd / 8B dd). Range +/- 256B.

    // We assume 16-bit opcodes.

    if (cond == -1) {
        // Unconditional BRA (A0 dd)
        if (dest != (mp_uint_t)-1) {
            // Label defined
            // Check range
            if (rel >= -4096 && rel <= 4094) {
               asm_sh_write_word16(as, 0xA000 | ((rel / 2) & 0xFFF));
               asm_sh_write_word16(as, 0x0009); // NOP
               return;
            }
        }
        // If not defined or out of range, ideally we use JMP.
        // But JMP requires register.
        // "Generic" assembler logic should handle long jumps if we report failure?
        // No, we must handle it.
        // But for now, we assume simple BRA works or emit placeholder.
        // Since we are in PASS logic, we can emit BRA and check size later?
        // But if size changes (BRA vs JMP), offsets change.
        // We assume BRA is enough for typical small scripts.

        asm_sh_write_word16(as, 0xA000 | ((rel / 2) & 0xFFF));
        asm_sh_write_word16(as, 0x0009); // NOP

    } else {
        // Conditional BT/BF
        // Range is small (+/- 256 bytes).
        // If out of range, we need to invert condition and branch over a long jump.
        // But that changes size.
        // We assume in range for now.

        uint16_t op;
        if (cond == ASM_SH_CC_EQ) op = 0x8900; // BT
        else if (cond == ASM_SH_CC_NE) op = 0x8B00; // BF
        else op = 0x8900; // Default to BT?

        asm_sh_write_word16(as, op | ((rel / 2) & 0xFF));
    }
}

// Stack ops
void asm_sh_mov_local_reg(asm_sh_t *as, int local_num, uint rd) {
    // If disp fits in 4 bits (0-60 bytes).
    int disp = local_num * 4;
    if (disp <= 60) {
        // MOV.L @(disp, Rm), Rn (5n md). n=dest, m=base, d=disp
        asm_sh_write_word16(as, 0x5000 | (rd << 8) | (15 << 4) | (disp / 4)); // MOV.L @(disp, R15), Rn
    } else {
        asm_sh_mov_reg_reg(as, ASM_SH_REG_R1, ASM_SH_REG_R15);
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R2, disp);
        asm_sh_add_reg_reg(as, ASM_SH_REG_R1, ASM_SH_REG_R2);
        asm_sh_write_word16(as, 0x6002 | (rd << 8) | (ASM_SH_REG_R1 << 4)); // MOV.L @Rm, Rn (6n m2)
    }
}

void asm_sh_mov_reg_local(asm_sh_t *as, uint rd, int local_num) {
    int disp = local_num * 4;
    if (disp <= 60) {
        // MOV.L Rm, @(disp, Rn) (1n md)
        asm_sh_write_word16(as, 0x1000 | (rd << 4) | ((disp / 4)) | (15 << 8));
    } else {
        asm_sh_mov_reg_reg(as, ASM_SH_REG_R1, ASM_SH_REG_R15);
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R2, disp);
        asm_sh_add_reg_reg(as, ASM_SH_REG_R1, ASM_SH_REG_R2);
        asm_sh_write_word16(as, 0x2002 | (ASM_SH_REG_R1 << 8) | (rd << 4)); // MOV.L Rm, @Rn (2n m2)
    }
}

// Memory ops
// MOV.L @(R0, Rm), Rn (0n mC)
void asm_sh_ldr_reg_reg(asm_sh_t *as, uint rd, uint rm, uint byte_offset) {
    if (byte_offset == 0) {
        asm_sh_write_word16(as, 0x6002 | (rd << 8) | (rm << 4)); // MOV.L @Rm, Rn
    } else {
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R0, byte_offset);
        asm_sh_write_word16(as, 0x000C | (rd << 8) | (rm << 4));
    }
}

// MOV.L Rm, @(R0, Rn) (0n m4)
void asm_sh_str_reg_reg(asm_sh_t *as, uint rd, uint rm, uint byte_offset) {
    if (byte_offset == 0) {
        asm_sh_write_word16(as, 0x2002 | (rm << 8) | (rd << 4)); // MOV.L Rm, @Rn
    } else {
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R0, byte_offset);
        asm_sh_write_word16(as, 0x0004 | (rm << 8) | (rd << 4));
    }
}

void asm_sh_ldrb_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x6000 | (rd << 8) | (rm << 4)); // MOV.B @Rm, Rn
}

void asm_sh_ldrh_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x6001 | (rd << 8) | (rm << 4)); // MOV.W @Rm, Rn
}

void asm_sh_strb_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x2000 | (rm << 8) | (rd << 4)); // MOV.B Rm, @Rn
}

void asm_sh_strh_reg_reg(asm_sh_t *as, uint rd, uint rm) {
    asm_sh_write_word16(as, 0x2001 | (rm << 8) | (rd << 4)); // MOV.W Rm, @Rn
}

void asm_sh_ldrh_reg_reg_offset(asm_sh_t *as, uint rd, uint rm, uint byte_offset) {
    asm_sh_mov_reg_i32(as, ASM_SH_REG_R0, byte_offset);
    asm_sh_write_word16(as, 0x000D | (rd << 8) | (rm << 4)); // MOV.W @(R0, Rm), Rn
}

// Others
void asm_sh_mov_reg_local_addr(asm_sh_t *as, uint rd, int local_num) {
    int disp = local_num * 4;
    asm_sh_mov_reg_reg(as, rd, ASM_SH_REG_R15);
    asm_sh_mov_reg_i32(as, ASM_SH_REG_R1, disp);
    asm_sh_add_reg_reg(as, rd, ASM_SH_REG_R1);
}

void asm_sh_mov_reg_pcrel(asm_sh_t *as, uint reg_dest, uint label) {
    // Load label address into reg_dest.
    // Address is PC relative?
    // Use MOVA @(disp, PC), R0?
    // MOVA (Move effective Address): 1100 0111 dddd dddd. R0 = (PC&~3) + 4 + disp*4.
    // We can use this to get address of constant or label if 8-bit disp.

    // For general label:
    mp_uint_t dest = as->base.label_offsets[label];
    // We can't easily do it.
    // Fallback: Load constant 0 (placeholder).
    (void)dest;
    asm_sh_mov_reg_imm8(as, reg_dest, 0);
}

void asm_sh_setcc_reg(asm_sh_t *as, uint rd, uint cond) {
    asm_sh_write_word16(as, 0x0029 | (rd << 8)); // MOVT Rn

    if (cond == ASM_SH_CC_NE) {
        asm_sh_cmp_reg_i8(as, rd, 0);
        asm_sh_write_word16(as, 0x0029 | (rd << 8)); // MOVT Rn
    }
}

void asm_sh_bl_ind(asm_sh_t *as, uint fun_id, uint reg_temp) {
    // MOV.L @(disp, R11), reg_temp
    int disp = fun_id * 4;
    if (disp <= 60) {
        asm_sh_write_word16(as, 0x5000 | (reg_temp << 8) | (REG_FUN_TABLE << 4) | (disp / 4));
    } else {
        asm_sh_mov_reg_i32(as, ASM_SH_REG_R0, disp);
        asm_sh_write_word16(as, 0x000E | (reg_temp << 8) | (REG_FUN_TABLE << 4)); // MOV.L @(R0, Rm), Rn
    }

    // JSR @reg_temp (4n 0B)
    asm_sh_write_word16(as, 0x400B | (reg_temp << 8));
    asm_sh_write_word16(as, 0x0009); // NOP
}

void asm_sh_bx_reg(asm_sh_t *as, uint reg_src) {
    // JMP @Rn (4n 2B)
    asm_sh_write_word16(as, 0x402B | (reg_src << 8));
    asm_sh_write_word16(as, 0x0009); // NOP
}

void asm_sh_str_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn) {
    asm_sh_mov_reg_reg(as, ASM_SH_REG_R0, rn);
    asm_sh_write_word16(as, 0x0006 | (rd << 8) | (rm << 4)); // MOV.L Rd, @(R0, Rm)
}

void asm_sh_strb_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn) {
    asm_sh_mov_reg_reg(as, ASM_SH_REG_R0, rn);
    asm_sh_write_word16(as, 0x0004 | (rd << 8) | (rm << 4)); // MOV.B Rd, @(R0, Rm)
}

void asm_sh_strh_reg_reg_reg(asm_sh_t *as, uint rd, uint rm, uint rn) {
    asm_sh_mov_reg_reg(as, ASM_SH_REG_R0, rn);
    asm_sh_write_word16(as, 0x0005 | (rd << 8) | (rm << 4)); // MOV.W Rd, @(R0, Rm)
}

#endif // MICROPY_EMIT_SH
