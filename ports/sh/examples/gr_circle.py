from gint import *

dgray(DGRAY_ON)
dupdate()

dclear(C_LIGHT)

dcircle(20, 20, 20, C_WHITE, C_BLACK )
dellipse(10, 10, 60, 50, C_LIGHTEN, C_DARKEN )
drect_border( 40, 40, 50, 50, C_DARK, 2, C_INVERT )
drect( 100, 0, 128, 50, C_BLACK )

print(dgray_enabled())
print(dgray_getdelays())

dupdate()
getkey()

dgray(DGRAY_OFF)
dupdate()

print(dgray_enabled())
