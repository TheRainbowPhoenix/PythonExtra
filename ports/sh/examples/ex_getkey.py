import gint

while True:
    ev = gint.getkey_opt(gint.GETKEY_DEFAULT, 5000)
    if ev.type == gint.KEYEV_NONE:
        print("no press after 5000 ms")
    else:
        print(ev)
    if ev.key == gint.KEY_EXIT and not ev.shift and not ev.alpha:
        break
