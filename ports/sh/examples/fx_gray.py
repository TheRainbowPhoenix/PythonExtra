from gint import *

status = dgray(DGRAY_ON)
print("dgray(DGRAY_ON) = ", status)
print("Gray mode activated: ", dgray_enabled())
print("dgray_getdelays()= ", dgray_getdelays())

dclear(C_WHITE)
dcircle(20, 20, 20, C_LIGHT, C_BLACK)
dellipse(10, 10, 60, 50, C_LIGHTEN, C_DARKEN)
drect_border(40, 40, 50, 50, C_DARK, 2, C_INVERT)
drect(100, 0, 128, 32, C_BLACK)
dupdate()

getkey()

status = dgray(DGRAY_OFF)
print("dgray(DGRAY_OFF) = ", status)
print("Gray mode activated: ", dgray_enabled())

dupdate()
