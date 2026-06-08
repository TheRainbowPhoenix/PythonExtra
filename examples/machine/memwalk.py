from gint import *
import machine
import cinput

# Ensure basic colors are defined
C_BLACK = C_RGB(0, 0, 0)
C_WHITE = C_RGB(31, 31, 31)
C_ACCENT = C_RGB(0, 15, 31)

def draw_ram(base_addr, step):
    """
    Reads from raw memory and renders it as RGB565 pixels.
    """
    dclear(C_BLACK)
    
    # We iterate over the physical screen dimensions (320x528)
    for y in range(0, DHEIGHT, step):
        # Pre-calculate the starting address of the current row (scanline)
        # Each pixel is 2 bytes (16-bit word), so a full screen width is 640 bytes
        row_ptr = base_addr + (y * DWIDTH * 2)
        
        for x in range(0, DWIDTH, step):
            addr = row_ptr + (x * 2)
            
            # Read the 16-bit color directly from memory
            color = machine.mem16[addr]
            
            # Draw it on the screen
            if step == 1:
                dpixel(x, y, color)
            else:
                drect(x, y, x + step - 1, y + step - 1, color)
                
    # Draw overlay HUD at the BOTTOM of the screen
    hud_bg = C_ACCENT
    drect(0, DHEIGHT - 30, DWIDTH, DHEIGHT, hud_bg)
    
    # Left side: Controls Info
    dtext(5, DHEIGHT - 22, C_WHITE, "Step: {} (+/-)  Jump: KBD".format(step))
    
    # Right side: Current Address
    addr_str = "0x{:08X}".format(base_addr)
    dtext(DWIDTH - 100, DHEIGHT - 22, C_WHITE, addr_str)
    dupdate()

def main():
    # RAM typically starts at 0x8c000000 on SH-4 architectures
    current_addr = 0x8c000000
    step = 4  # Start with a step of 4 for very fast rendering
    
    # Initial render
    draw_ram(current_addr, step)
    
    while True:
        # getkey() pauses execution until a key is pressed
        ev = getkey()
        
        if ev.type == KEYEV_DOWN:
            if ev.key == KEY_DOWN:
                # Scroll down safely by 40 lines
                current_addr += 40 * DWIDTH * 2
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_UP:
                # Scroll up safely by 40 lines
                current_addr -= 40 * DWIDTH * 2
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_6 or ev.key == KEY_RIGHT:
                # Jump one full screen down
                current_addr += DHEIGHT * DWIDTH * 2
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_4 or ev.key == KEY_LEFT:
                # Jump one full screen up
                current_addr -= DHEIGHT * DWIDTH * 2
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_PLUS or ev.key == KEY_EXP:
                # Increase detail / Decrease step
                step = max(1, step - 1)
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_MINUS or ev.key == KEY_NEG:
                # Decrease detail / Increase step (faster render)
                step = min(8, step + 1)
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_KBD:
                # Open cinput text dialog to enter an address manually
                try:
                    # Provide the current address as the default text (strip '0x')
                    default_text = "{:08X}".format(current_addr)
                    res = cinput.input("Go to Hex Address:", default_text)
                    
                    if res:
                        # Parse the hex string back to an integer
                        new_addr = int(res.strip(), 16)
                        # Ensure word alignment (mask out the lowest bit)
                        current_addr = new_addr & ~1
                except Exception:
                    pass # Ignore invalid hex inputs
                    
                # Redraw the screen with the new address
                draw_ram(current_addr, step)
                
            elif ev.key == KEY_EXIT:
                break

# Execute main function
main()