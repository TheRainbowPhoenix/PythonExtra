from gint import *
import math

if DWIDTH==128:
    C_RGB = lambda r, g, b: 0
    _ = lambda x, y: x
else:
    _ = lambda x, y: y

dclear(C_WHITE)

c1 = _(C_BLACK, C_RGB(24, 24, 24))
c2 = _(C_BLACK, C_BLACK)
c3 = _(C_NONE, C_RGB(24, 24, 24))

x1 = _(5,20)
x2 = _(40,90)
x3 = _(120,360)
y1 = _(2,20)
y2 = _(19,60)
y3 = _(30,135)
y4 = _(45,170)
y5 = _(50,190)

xp = _(90,200)
yp = _(25,100)
rp = _(20,50)

drect(x1, y1, x2, y2, c1)
drect_border(x1, y3, x2, y4, c3, 2, c2)

dvline(x3, c1)
dvline(x3+2, c1)
dvline(x3+4, c1)
dhline(y5, c2)
dhline(y5+2, c2)
dhline(y5+4, c2)

p = [2*math.pi*i/7 for i in range(7)]
p = [(int(xp+rp*math.cos(a)), int(yp+rp*math.sin(a))) for a in p]

for i in range(7):
    dline(*p[i], *p[(i+2)%7], C_BLACK)

dupdate()
getkey()
