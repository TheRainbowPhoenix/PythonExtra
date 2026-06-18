# Report: Porting MicroPython Native Emitter to SuperH (SH4)

This report details the steps taken to port the MicroPython native code emitter to the SuperH (SH4) architecture, specifically targeting the SH4A-NOFPU variant used in Casio calculators.

## Overview

The native code emitter in MicroPython allows Python code to be compiled to machine code at runtime, offering significant performance improvements over bytecode interpretation. This is achieved through a generic emitter interface (`emitnative.c`) which relies on an architecture-specific assembler implementation.

## Architecture Specifics (SH4)

*   **Registers**: SH4 has 16 general-purpose 32-bit registers (R0-R15).
    *   `R0`: Return value, often used as accumulator/index.
    *   `R1-R3`: Scratch registers.
    *   `R4-R7`: Argument registers (first 4 arguments).
    *   `R8-R13`: Callee-saved registers.
    *   `R14`: Frame Pointer (callee-saved).
    *   `R15`: Stack Pointer.
    *   `PR`: Procedure Register (return address).
    *   `MACH`, `MACL`: Multiply-accumulate registers.
    *   `SR`: Status Register (T-bit for conditions).
*   **Instruction Set**: Fixed 16-bit instruction length.
*   **Endianness**: Big Endian (`-mb`) is used for the Casio SH4 port.
*   **Calling Convention**: Standard GCC SH calling convention is used.

## Implementation Details

### 1. Assembler Interface (`py/asmsh.h` & `py/asmsh.c`)

A new assembler module was created to handle SH4 instruction generation.

*   **`py/asmsh.h`**: Defines the `asm_sh_t` structure, SH4 register constants, and mapping macros that translate MicroPython's generic `ASM_...` operations to `asm_sh_...` functions.
    *   Mapped `REG_ARG_1`..`REG_ARG_4` to `R4`..`R7`.
    *   Mapped `REG_RET` to `R0`.
    *   Mapped `REG_LOCAL_1`..`REG_LOCAL_3` to `R8`..`R10` (Callee-saved).
*   **`py/asmsh.c`**: Implements the actual instruction encoding.
    *   Implemented generic `asm_sh_mov_reg_reg`, `asm_sh_add_reg_reg`, etc.
    *   Implemented `asm_sh_entry` and `asm_sh_exit` to manage the stack frame, saving `PR`, `R14`, and callee-saved registers `R8`-`R13`.
    *   Implemented **cache flushing** using `__builtin___clear_cache` in `py/emitglue.c` to ensure coherency between Instruction and Data caches after code generation.
    *   Implemented 32-bit immediate loading using PC-relative addressing with inline constants (`MOV.L @(0, PC)`), ensuring 4-byte alignment and jumping over the constant.
    *   Implemented control flow (`BRA`, `BT`, `BF`) using `label_offsets` provided by the multi-pass emitter to calculate relative offsets.

### 2. Native Emitter Wrapper (`py/emitnsh.c`)

This file acts as the glue between the generic `emitnative.c` and the SH4 assembler.
*   Defines `N_SH` to enable SH4-specific logic in `emitnative.c`.
*   Includes `py/asmsh.h` to provide the assembler interface.
*   Defines `NLR_BUF_IDX_LOCAL_1` to allow the emitter to optimize exception handling by storing state in callee-saved registers.

### 3. Build System Integration

*   **`py/py.mk`**: Updated to include `asmsh.o` and `emitnsh.o` in the core object list.
*   **`py/emit.h`**: Added declarations for the SH4 emitter method table and constructor/destructor.
*   **`py/emitglue.c`**: Added logic to handle cache flushing for `MICROPY_EMIT_SH`.
*   **`py/mpconfig.h`**: Added default definition `MICROPY_EMIT_SH` (0).
*   **`ports/sh/mpconfigport.h`**: Enabled `MICROPY_EMIT_SH` (1) for the SH port.

## Verification

The implementation covers the core set of instructions required by the native emitter:
*   Data movement (register-register, immediate-register, stack load/store).
*   Arithmetic (ADD, SUB, MUL).
*   Bitwise (AND, OR, XOR, NOT).
*   Shifts (LSL, LSR, ASR).
*   Comparisons and Branching (CMP, BT/BF, BRA) with label resolution.
*   Function calls (JSR).

### Limitations / Future Work

*   **Floating Point**: The current implementation assumes integer operations (NOFPU). Floating point operations in native code are not yet supported.
*   **Large Functions**: Branch offsets are limited. `BRA` is used which supports +/- 4KB range. For larger functions, trampoline or register-based jumps would be required if the code size exceeds this limit.
*   **Optimization**: The generated code is functional but not highly optimized.

## How to Compile

1.  Navigate to `ports/sh`.
2.  Run `make`.
3.  The build system will now include `asmsh.c` and `emitnsh.c` and link them into the firmware.

## Conclusion

The native code emitter infrastructure is now in place for the SH4 port. This enables the usage of `@micropython.native` and `@micropython.viper` decorators to accelerate performance-critical Python code on Casio calculators.
