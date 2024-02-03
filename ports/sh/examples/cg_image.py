from gint import *
import math

palette = b'\x00\x00\xf8\x00\xff\xff'
data = b'\x80\x81\x80\x81\x82\x81\x80\x81\x80'
img = image(IMAGE_P8_RGB565, color_count=3, width=3, height=3,
    stride=3, data=data, palette=palette)
print(img)

img_alpha = image_p8_rgb565a(3, 3, data, palette)
print(img_alpha)

# h ∈ [0,360), s ∈ [0,1], l ∈ [0,1]
def hsl2rgb(H, S, L):
    C = (1 - abs(2*L - 1)) * S
    Hp = H / 60
    X = C * (1 - abs(Hp % 2 - 1))
    R, G, B = [(C,X,0), (X,C,0), (0,C,X), (0,X,C), (X,0,C), (C,0,X)][int(Hp)]
    m = L - C / 2
    return (R + m, G + m, B + m)

gradient_data = bytearray(32*32*2)
for y in range(32):
    for x in range(32):
        i = (32 * y + x) * 2
        lx, ly = x-15.5, y-15.5
        radius = (lx*lx + ly*ly) / (16**2)
        if radius > 1:
            gradient_data[i] = 0x00
            gradient_data[i+1] = 0x01
            continue

        h = math.atan2(-ly, lx) * 180 / math.pi
        h = h if h >= 0 else h + 360
        s = radius
        l = 0.5
        r, g, b = hsl2rgb(h, s, l)
        color = C_RGB(int(r*32), int(g*32), int(b*32))
        gradient_data[i] = color >> 8
        gradient_data[i+1] = color & 0xff

gradient = image_rgb565a(32, 32, gradient_data)
print(gradient)

from cg_image_puzzle import *
print(puzzle)

# Take graphics control
dupdate()

h = DWIDTH // 2
dclear(C_WHITE)
drect(h, 0, DWIDTH-1, DHEIGHT-1, C_RGB(28,28,28))

dimage(10, 10, img)
dimage(15, 10, img)
dimage(20, 10, img)
dimage(h+10, 10, img_alpha)
dimage(h+15, 10, img_alpha)
dimage(h+20, 10, img_alpha)
dimage(h-16, 20, gradient)
dimage(40, 100, puzzle)

x, y = 110, 100
hw = puzzle.width // 2
hh = puzzle.height // 2
dsubimage(x-1, y-1, puzzle, 0, 0, hw, hh)
dsubimage(x+hw+1, y-1, puzzle, hw, 0, hw, hh)
dsubimage(x-1, y+hh+1, puzzle, 0, hh, hw, hh)
dsubimage(x+hw+1, y+hh+1, puzzle, hw, hh, hw, hh)

dupdate()
getkey()
