from gint import *
dclear(C_WHITE)
for y in range(10):
    for x in range(10):
        if (x^y) & 1:
            dpixel(x, y, C_BLACK)
dupdate()
getkey()
