from gint import *

def clear():
    dclear(C_WHITE)
    dtext(2, 2, C_BLACK, "Touch: draw | EXE: clear | Clear: quit")

clear()
dupdate()

x, y = -1, -1

while True:
    ev = pollevent()
    while ev.type != KEYEV_NONE:
        if ev.type == KEYEV_DOWN and ev.key == KEY_EXE:
            clear()
        if ev.type == KEYEV_TOUCH_DOWN:
            x, y = ev.x, ev.y
        if ev.type == KEYEV_TOUCH_DRAG:
            dline(x, y, ev.x, ev.y, C_BLACK);
            x, y = ev.x, ev.y
        ev = pollevent()
    dupdate()
