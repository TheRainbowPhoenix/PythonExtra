.text

/* template for syscalls */
#define SYSCALL(NAME, NUM)  \
.global NAME;               \
NAME:                       \
    mov.l   .SYSCALL, r2;   \
    mov.l   1f, r0;         \
    jmp     @r2;            \
    nop;                    \
1:  .long NUM

SYSCALL(_CASIOWIN_GetBatteryType, 0x12d5)
SYSCALL(_CASIOWIN_GetMainBatteryVoltage, 0x1186)

.SYSCALL: .long 0x80020070
