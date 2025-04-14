from shmup import *
from numworks import *
from gint import *

dclear(C_WHITE)

dtext(10,10,C_BLACK,"Hello before changing font")

print("TEST : Shmup font (Proportional)")
dfont(font_shmup)
dtext(10,20,C_BLACK,"Hello after changing font - Shmup font - Proportional")

print("TEST : Shmup font (Proportional)")
dfont(font_numworks)
dtext(10,30,C_BLACK,"Hello after changing font - Numworks font - Monospaced")

print("TEST : Back to gint's default font")
dfont(None)
dtext(10,50,C_BLACK,"Hello after changing font - Back to gint's default font")

dupdate()
getkey()