from gint import *
import machine

# Ensure basic colors are defined
C_BLACK = C_RGB(0, 0, 0)
C_WHITE = C_RGB(31, 31, 31)
C_ACCENT = C_RGB(0, 15, 31)

def detect_pointer_addr():
    """Reads the OS version from 0x80020020 to determine the VRAM pointer address."""
    try:
        # Read 15 bytes to cover up to 0x8002002e
        chars = [chr(machine.mem8[0x80020020 + i]) for i in range(15)]
        os_ver = "".join(chars)
        
        if os_ver.startswith("02.01.2000"):
            return "v2", os_ver
        else:
            return "v3", os_ver
    except Exception:
        return "??", "Unknown"

class PegBitmap:
    """Parses a PegBitmap C struct directly from memory."""
    def __init__(self, addr):
        self.addr = addr
        # Memory mappings based on the 16-byte SH-4 struct layout:
        self.uFlags      = machine.mem8[addr + 0]
        self.uBitsPx     = machine.mem8[addr + 1]
        self.wWidth      = machine.mem16[addr + 2]
        self.wHeight     = machine.mem16[addr + 4]
        # offset 6 is 2 bytes of padding (_pad)
        self.dTransColor = machine.mem32[addr + 8]
        self.pStart      = machine.mem32[addr + 12]

# Determine pointer address
OS_NAME, OS_VERSION = detect_pointer_addr()

def main():
    print(OS_NAME, OS_VERSION)

# Execute main function
main()
