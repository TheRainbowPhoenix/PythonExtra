import machine

def read_username():
    USERNAME_ADDR = 0x8C1BE984
    username_bytes = bytearray()

    # Read until null byte or max 88 chars
    for i in range(88):
        char_val = machine.mem8[USERNAME_ADDR + i]
        if char_val == 0:
            break
        username_bytes.append(char_val)

    try:
        return username_bytes.decode('utf-8')
    except:
        # Fallback if invalid utf-8
        return repr(username_bytes)

def read_spu_info():
    # SPU address from gint/mpu/spu.h: #define SH7305_SPU (*(spu_t *)0xfe2ffc00)
    SPU_BASE = 0xfe2ffc00

    # Offsets based on the spu_t struct:
    # PBANKC0 is at offset 0
    # PBANKC1 is at offset 4
    # ...
    # SPUADR is at offset 0x24 (36)
    # ENDIAN is at offset 0x28 (40)

    pbankc0 = machine.mem32[SPU_BASE + 0x00]
    pbankc1 = machine.mem32[SPU_BASE + 0x04]
    xbanc0 = machine.mem32[SPU_BASE + 0x10]  # Note: pad(0x8) after PBANKC1 (4+4+8 = 16 = 0x10)

    print("--- SPU Info ---")
    print(f"PBANKC0: 0x{pbankc0:08x}")
    print(f"PBANKC1: 0x{pbankc1:08x}")
    print(f"XBANKC0: 0x{xbanc0:08x}")

    # DSP0 is at 0xfe2ffd00
    DSP0_BASE = 0xfe2ffd00
    dsp0_iemaskc = machine.mem32[DSP0_BASE + 0x18] # DSPRST(4) + DSPCORERST(4) + DSPHOLD(4) + DSPRESTART(4) + pad(8) = 0x18
    print("--- DSP0 Info ---")
    print(f"IEMASKC: 0x{dsp0_iemaskc:08x}")

def main():
    print("Casio SPU/DSP and Username Reader Demo")
    print("--------------------------------------")

    username = read_username()
    print(f"Calculator Username: '{username}'")
    print("")

    read_spu_info()
    print("")
    print("Demo complete.")

if __name__ == "__main__":
    main()
