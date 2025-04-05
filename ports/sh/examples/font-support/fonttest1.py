from shmup import *
from gint import *

dclear(C_WHITE)

print("TEST : Shmup font (Proportional)")
dfont(font_shmup)
dtext(10,10,C_RED,"Hello")

dupdate()
getkey()