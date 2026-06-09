from gint import *
import machine

class SysFont:
    def __init__(self, addr):
        """Initializes a PEG Font by reading the struct at the given memory address."""
        self.addr = addr
        
        # Read properties according to PegFont struct
        # UCHARs
        self.uType = machine.mem8[addr + 0]
        self.uAscent = machine.mem8[addr + 1]
        self.uDescent = machine.mem8[addr + 2]
        self.uHeight = machine.mem8[addr + 3]
        
        # WORDs (uint16)
        self.wBytesPerLine = machine.mem16[addr + 4]
        self.wFirstChar = machine.mem16[addr + 6]
        self.wLastChar = machine.mem16[addr + 8]
        
        # POINTERS (uint32)
        # Note: In C on SH-4, a 32-bit pointer must be aligned to a 4-byte boundary.
        # wLastChar ends at offset 9, so offsets 10 & 11 are structural padding!
        self.pOffsets = machine.mem32[addr + 12]
        self.pNext = machine.mem32[addr + 16]
        self.pData = machine.mem32[addr + 20]

    def get_char_width(self, ch_ord):
        """Returns the width of the character in pixels."""
        if ch_ord < self.wFirstChar or ch_ord > self.wLastChar:
            return 0
            
        idx = ch_ord - self.wFirstChar
        off1 = machine.mem16[self.pOffsets + (idx * 2)]
        off2 = machine.mem16[self.pOffsets + ((idx + 1) * 2)]
        return off2 - off1

    def draw_string(self, x, y, string, color=C_BLACK):
        """Draws a full string using the loaded font with basic newline support."""
        xp, yp = x, y
        for ch in string:
            ch_ord = ord(ch)
            if ch_ord == 10:  # Handle newline '\n'
                xp = x
                yp += self.uHeight + 2
                continue
                
            w = self.get_char_width(ch_ord)
            if w > 0:
                self._draw_char(xp, yp, ch_ord, color, w)
                xp += w + 1  # 1px spacing between chars
                
    def _draw_char(self, x, y, ch_ord, color, width):
        """Draws a single character using horizontal line spans for speed."""
        idx = ch_ord - self.wFirstChar
        start_bit = machine.mem16[self.pOffsets + (idx * 2)]
        bpl_bits = self.wBytesPerLine * 8
        data_ptr = self.pData
        
        for row in range(self.uHeight):
            row_start_bit = start_bit + row * bpl_bits
            
            span_start = -1
            for col in range(width):
                bit_off = row_start_bit + col
                
                # Fetch bit directly from memory
                val = (machine.mem8[data_ptr + (bit_off >> 3)] >> (7 - (bit_off & 7))) & 1
                
                if val:
                    if span_start == -1: 
                        span_start = col
                else:
                    if span_start != -1:
                        # Draw a horizontal line segment when the span ends
                        dline(x + span_start, y + row, x + col - 1, y + row, color)
                        span_start = -1
                        
            # Close off any remaining span at the end of the width
            if span_start != -1:
                dline(x + span_start, y + row, x + width - 1, y + row, color)

def main():
    dclear(C_WHITE)
    dtext(10, 10, C_BLACK, "Loading System Fonts...")
    dupdate()
    
    try:
        # Construct font wrappers pointing to internal OS addresses
        # Fonts are at 8c1a70cc, 8c1a712c, 8c1a71a4 # 8c1a75c4 # 8c1a718c
        f1 = SysFont(0x8c1a70cc)
        f2 = SysFont(0x8c1a718c)
        f3 = SysFont(0x8c1a71a4)
        
        dclear(C_WHITE)
        
        # Test Font 1
        f1.draw_string(5, 5, "System Font 1 (0x8c1a70cc):\nHello World! AaBbCc", C_BLACK)
        
        # Test Font 2
        f2.draw_string(5, 70, "System Font 2 (0x8c1a718c):\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n0123456789", C_BLUE)
        
        # Test Font 3
        f3.draw_string(5, 170, "System Font 3 (0x8c1a71a4):\nTesting MicroPython PEG renderer", C_RED)
        
    except Exception as e:
        dclear(C_WHITE)
        dtext(10, 10, C_BLACK, "Crash during font access!")
        dtext(10, 30, C_BLACK, str(e))
        
    dtext(10, 500, C_BLACK, "Press any key to exit.")
    dupdate()
    getkey()

main()