from numworks import *
from gint import *

dclear(C_WHITE)

dtext(10,10,C_BLUE,"Hello before changing font")

print("TEST : Shmup font (Proportional)")
dfont(font_numworks)
dtext(10,50,C_RED,"Hello after changing font")

dupdate()
getkey()