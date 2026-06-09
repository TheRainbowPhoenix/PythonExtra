from gint import *
import machine
import time

def get_string(addr, max_len=8):
    """Reads a null-terminated ASCII string of up to max_len bytes from memory."""
    chars = []
    for i in range(max_len):
        val = machine.mem8[addr + i]
        if val == 0: 
            break
        # Filter for printable ASCII
        if 32 <= val < 127:
            chars.append(chr(val))
        else:
            chars.append('.')
    return "".join(chars)

def main():
    # Setup simple light theme colors
    bg_color = C_WHITE
    text_color = C_BLACK
    header_color = C_BLUE if 'C_BLUE' in globals() else C_BLACK
    
    # 1. Clear screen
    dclear(bg_color)
    dtext(10, 10, header_color, "--- MCS MEMORY TEST ---")
    dupdate()
    
    print("\nReading MCS Memory on ClassPad CP400 ...")
    
    DIR_BASE = 0x8CF80100
    MAX_DIRS = 0x87  # Maximum directories to check (135)
    
    y = 30  # Text drawing cursor Y coordinate
    items_shown = 0
    max_items = 25  # Limit rendering to avoid screen overflow
    
    for i in range(MAX_DIRS):
        if items_shown >= max_items:
            break
            
        dir_addr = DIR_BASE + i * 16
        dir_name = get_string(dir_addr, 8)
        
        # Skip empty directory entries
        if not dir_name:
            continue
            
        data_ptr = machine.mem32[dir_addr + 8]
        var_num = machine.mem16[dir_addr + 12]
        
        # Format info and output to console
        dir_info = "[Dir] {} ({} vars)".format(dir_name, var_num)
        print(dir_info)
        
        # Render directory onto the screen
        dtext(10, y, text_color, dir_info)
        y += 16
        items_shown += 1
        
        # Read variables inside this folder (limit to first few to keep it basic)
        vars_to_show = min(var_num, 3)
        for j in range(vars_to_show):
            if items_shown >= max_items:
                break
                
            var_addr = data_ptr + j * 20
            var_name = get_string(var_addr, 8)
            var_size = machine.mem32[var_addr + 12]
            var_type = machine.mem8[var_addr + 16]
            
            var_info = "  -> {} | Sz: {} | Ty: {}".format(var_name, var_size, var_type)
            print(var_info)
            
            # Render variables indented on screen
            dtext(25, y, text_color, var_info)
            y += 16
            items_shown += 1
            
        if var_num > vars_to_show:
            more_info = "  -> ... and {} more".format(var_num - vars_to_show)
            print(more_info)
            dtext(25, y, text_color, more_info)
            y += 16
            items_shown += 1
            
    # Draw instructions and update display
    dtext(10, 500, header_color, "Press any key to exit.")
    dupdate()
    
    print("\nScan complete. Press any key on the calculator to exit.")
    getkey()

# Call the main entrypoint directly (avoids main guard bug)
main()