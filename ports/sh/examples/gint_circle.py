from gint import *

if DWIDTH==128:
    C_RGB = lambda r, g, b: 0
    _ = lambda x, y: x
else:
    _ = lambda x, y: y

dclear(C_WHITE)

fill1 = _(C_NONE, C_RGB(24, 24, 24))
fill2 = _(C_BLACK, C_RGB(24, 24, 24))

x1 = _(5,20)
x2 = _(40,90)
x3 = _(64,120)
x4 = _(97,220)
y1 = _(2,20)
y2 = _(19,60)
w1 = _(26,60)
h1 = _(15,30)

dellipse(x1, y1, x1+w1, y1+h1, fill1, C_BLACK)
dellipse(x1, y2, x1+w1, y2+4, fill1, C_BLACK)
dellipse(x1, y2+_(6,10), x1+w1, y2+_(6,10), fill1, C_BLACK)
dellipse(x2, y1, x2+4, y1+h1, fill1, C_BLACK)
dellipse(x2+10, y1, x2+20, y1+h1, fill1, C_BLACK)
dellipse(x3, y1, x3+w1, y1+h1, C_NONE, C_BLACK)
dellipse(x4, y1, x4+w1, y1+h1, fill2, C_NONE)

y3 = _(40,135)
y4 = _(55,170)
r = _(10,20)
x1 += w1 // 2

dcircle(x1, y3, w1//2, fill1, C_BLACK)
dcircle(x1, y4+2, 2, fill1, C_BLACK)
dcircle(x1, y4+_(6,10), 0, fill1, C_BLACK)
dcircle(x2+r, y3, r, fill1, C_BLACK)
dcircle(x3+w1//2, y3, r, C_NONE, C_BLACK)
dcircle(x4+w1//2, y3, r, fill2, C_NONE)

dupdate()
getkey()
