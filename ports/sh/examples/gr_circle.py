from gint import *

returnvalue1 = dgray(DGRAY_ON)
print( "Return from dgray(DGRAY_ON) = ", returnvalue1 )
print( "Module dgray activated : ", dgray_enabled())

dupdate()

dclear(C_LIGHT)

dcircle(20, 20, 20, C_WHITE, C_BLACK )
dellipse(10, 10, 60, 50, C_LIGHTEN, C_DARKEN )
drect_border( 40, 40, 50, 50, C_DARK, 2, C_INVERT )
drect( 100, 0, 128, 50, C_BLACK )


print( "dgray_getdelays()= ", dgray_getdelays())

dupdate()
getkey()

returnvalue2 = dgray(DGRAY_OFF)
print( "Return from dgray(DGRAY_OFF) = ", returnvalue2 )
print( "Module dgray activated : ", dgray_enabled())

dupdate()

