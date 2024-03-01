from gint import *

dclear(C_WHITE)

listvert = [0,0,127,0,127,63,0,63]
fill = C_RED
border = C_BLUE
dpoly(listvert, fill, border)

listvert2 = [100, 50, 200, 50, 225, 75, 200, 100, 100, 100, 75, 75]
listvert3 = [100, 100, 200, 100, 225, 125, 200, 150, 100, 150, 75, 125]
dpoly(listvert2, C_NONE, C_BLACK)
dpoly(listvert3, C_GREEN, C_NONE)

dupdate()
getkey()
