from casioplot import *

B=(0,0,0)
W=(255,255,255)

clear_screen()
set_pixel(1, 1, B)

for x in range(10):
    set_pixel(x+1,3,B)

for y in range(40):
    for x in range(10):
        set_pixel(x+1,y+5,B)

draw_string(55,1,"Hello,",B,"large")
draw_string(55,31,"medium",B,"medium")
draw_string(55,51,"world!",B,"small")

img = b'\x04@"\x00\x0e\xa6W\x00\x0e\xafW\x00\x05\x1f\x89\x00u/J\xc0\xf5F*\xe0'+\
      b'\xe4\xc92\xe0\x8cP\xa1 \x8c{\xe1 \x8c\xbf\xb1 \x8c\xb1\xb1 \x9c\x950'+\
      b'\xa0\x9e\xb1\x90\xa0\x8e\x7f\xc3 \xc3\xfb\xfc`\xe0\x1d\xc0\xe0\xbf\x12'+\
      b'\x1f\xa0_\t\x1f@ \x89 \x80\x1f\x92?\x00'

for y in range(20):
    for x in range(27):
        offset = y * 4 + (x // 8)
        bit = 0x80 >> (x & 7)
        if img[offset] & bit:
            set_pixel(x+20,y+2,B)

show_screen()

# TODO: Don't require a wait after casioplot ends
import gint
gint.getkey()
