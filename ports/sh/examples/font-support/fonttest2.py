from numworks import *
from gint import *

dclear(C_WHITE)

print("TEST : Numworks font (Monospaced)")
dfont(font_numworks)
dtext(10,50,C_BLUE,"Hello")

dupdate()
getkey()