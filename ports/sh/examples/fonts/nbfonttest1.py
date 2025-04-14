from shmup import *
from gint import *

dclear(C_WHITE)

dtext(10,10,C_BLACK,"Hello before changing font")

print("TEST : Shmup font (Proportional)")
dfont(font_shmup)
dtext(10,50,C_BLACK,"Hello after changing font")

dupdate()
getkey()