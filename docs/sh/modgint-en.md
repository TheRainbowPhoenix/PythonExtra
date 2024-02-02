# `gint`: Wrapper for the gint library

PythonExtra is developed with the [fxSDK](https://gitea.planet-casio.com/Lephenixnoir/fxsdk) and uses the [gint kernel](https://gitea.planet-casio.com/Lephenixnoir/gint) as a runtime. The Python module `gint` provides access to gint's internal functions for rendering, keyboard, and more. Since gint has many versatile functions with good performance, it is beneficial to use it instead of e.g. `casioplot` or `turtle`.

The `gint` module tries to match its API with the original API of the C library, which is why few functions use keyword arguments or overloading. There are a few differences documented at the end of this document. For details not described in this document, one can refer to [gint's header files](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint) which are always applicable unless this document explicitly says otherwise.

All constants, functions, etc. discussed here are in the `gint` module.

```py
import gint
# or:
from gint import *
```

## Keyboard input

Reference headers: [`<gint/keyboard.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/keyboard.h) and [`<gint/keycodes.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/keycodes.h).

The module provides integer constants to refer to keyboard keys, with the following names:

|`KEY_F1`    |`KEY_F2`    |`KEY_F3`    |`KEY_F4`    |`KEY_F5`    |`KEY_F6`     |
|`KEY_SHIFT` |`KEY_OPTN`  |`KEY_VARS`  |`KEY_MENU`  |`KEY_LEFT`  |`KEY_UP`     |
|`KEY_ALPHA` |`KEY_SQUARE`|`KEY_POWER` |`KEY_EXIT`  |`KEY_DOWN`  |`KEY_RIGHT`  |
|`KEY_XOT`   |`KEY_LOG`   |`KEY_LN`    |`KEY_SIN`   |`KEY_COS`   |`KEY_TAN`    |
|`KEY_FRAC`  |`KEY_FD`    |`KEY_LEFTP` |`KEY_RIGHTP`|`KEY_COMMA` |`KEY_ARROW`  |
|`KEY_7`     |`KEY_8`     |`KEY_9`     |`KEY_DEL`   |`KEY_ACON`  |             |
|`KEY_4`     |`KEY_5`     |`KEY_6`     |`KEY_MUL`   |`KEY_DIV`   |             |
|`KEY_1`     |`KEY_2`     |`KEY_3`     |`KEY_ADD`   |`KEY_SUB`   |             |
|`KEY_0`     |`KEY_DOT`   |`KEY_EXP`   |`KEY_NEG`   |`KEY_EXE`   |             |

There are also a few aliases:

- `KEY_X2` is an alias for `KEY_SQUARE` ;
- `KEY_CARET` is an alias for `KEY_POWER` ;
- `KEY_SWITCH` is an alias for `KEY_FD` ;
- `KEY_LEFTPAR` is an alias for `KEY_LEFTP` ;
- `KEY_RIGHTPAR` is an alias for `KEY_RIGHTP` ;
- `KEY_STORE` is an alias for `KEY_ARROW` ;
- `KEY_TIMES` is an alias for `KEY_MUL` ;
- `KEY_PLUS` is an alias for `KEY_ADD` ;
- `KEY_MINUS` is an alias for `KEY_SUB`.

### Keyboard events

```
key_event:
  .time  -> int
  .mod   -> bool
  .shift -> bool
  .alpha -> bool
  .type  -> KEYEV_NONE | KEYEV_DOWN | KEYEV_UP | KEYEV_HOLD
  .key   -> KEY_*
```

gint communicates information about keyboard activity through _events_. Events indicate when a key (`.key` field) is pressed, repeated or released (`.type` field equal to `KEYEV_DOWN`, `KEYEV_HOLD` and `KEYEV_UP` respectively), when (`.time` field), and whether modifiers (SHIFT, `.shift` field, and ALPHA, `.alpha` field) where active at that time.

(The `.mod` field isn't very interesting, and the `KEYEV_NONE` value for the `.type` field is discussed later with `pollevent()`.)

The functions `getkey()`, `getkey_opt()`, `pollevent()` and `waitevent()` all return events.

### Waiting for a key press

```py
getkey() -> key_event
```

The function `getkey()` pauses the program until a key is pressed or repeated, and returns the associated event (which is always of type `KEYEV_DOWN` or `KEYEV_HOLD`). By default, only arrow keys are repeated, once after 400 ms, then every 40 ms.

A few things can happen while `getkey()` is waiting. The user can press SHIFT or ALPHA which will trigger modifiers and affect the `.shift` and `.alpha` fields of the returned event. The user can also go to the main menu by pressing MENU or turn the calculator off with SHIFT+AC/ON.

_Example._ In a selection menu with N possible items, one could navigate with the up and down arrow keys, jump to the top or bottom with SHIFT up and SHIFT down, and validate with EXE.

```py
ev = getkey()
if ev.key == KEY_EXE:
    pass       # Validate
elif ev.key == KEY_UP and ev.shift:
    pos = 0    # Jump to top
elif ev.key == KEY_DOWN and ev.shift:
    pos = N-1  # Jump to bottom
elif ev.key == KEY_UP and pos > 0:
    pos -= 1   # Move one place up
elif ev.key == KEY_DOWN and pos < N-1:
    pos += 1   # Move one place down
```

### Reading keyboard events in real time

```py
pollevent() -> key_event
waitevent() -> key_event
clearevents() -> None
```

gint records keyboard activity in the background while the program is running. Events are placed in a queue until the program reads them. This is how `getkey()` learns about keyboard activity, for example.

The `pollevent()` function provides direct access to events. `pollevent()` returns the oldest event that hasn't yet been read by the program. If there are no events waiting to be read, `pollevent()` returns a "fake" event with type `KEYEV_NONE` to indicate that the queue is empty.

Since `pollevent()` returns instantly, it can be used to read keyboard activity without pausing the program.

_Example._ A game loop could, at every frame, read all pending events to determine when the player pressed the SHIFT key (in this example the "action" key) to perform an action.

```py
# Render game...

while True:
    ev = pollevent()
    if ev.type == KEYEV_NONE:
        break  # We're done reading events
    if ev.type == KEYEV_DOWN and ev.key == KEY_SHIFT:
        pass   # The SHIFT key was just pressed!
    # Implicitly ignores other keys

# Simulate game...
```

The `waitevent()` function operates similarly, but if there are no pending events it waits for something to happen before returning. It is used quite rarely because in waiting situations one usually uses `getkey()` instead.

The function `clearevents()` reads and ignores all events, i.e. it "throws away" all the information about recent keyboard activity. It is useful to know the immediate state of the keyboard with `keydown()'` (see below). `clearevents()` is equivalent to the following definition:

```py
def clearevents():
    ev = pollevent()
    while ev.type != KEYEV_NONE:
        ev = pollevent()
```

### Reading the immediate state of the keyboard

```py
keydown(key: int) -> bool
keyup(key: int) -> bool
keydown_all(*keys: [int]) -> bool
keydown_any(*keys: [int]) -> bool
```

After events have been read and the event queue is empty, one can query the immediate state of keys with the `keydown()` function. `keydown(k)` returns `True` if key `k` is currently pressed, `False` otherwise. This function only works **after events have been read**, which is usually done either with `pollevent()` or with `clearevents()`.

_Example._ A game loop could check the state of the left/right keys at every frame to move the player.

```py
while True:
    ev = pollevent()
    # ... same thing as the pollevent() example

if keydown(KEY_LEFT):
    player_x -= 1
if keydown(KEY_RIGHT):
    player_x += 1
```

`keyup()` returns the opposite of `keydown()`. `keydown_all()` takes a series of keys as parameters and returns `True` if they are all pressed. `keydown_any()` is similar and returns `True` if at least one of the listed keys is pressed.

### Quickly querying key state changes

```py
cleareventflips() -> None
keypressed(key: int) -> bool
keyreleased(key: int) -> bool
```

`keydown()` only tells whether keys are pressed at a given time; it cannot be used to check when keys change from the released state to the pressed state or the other way around. To do this, one must either read individual events (which can be annoying) or use the functions described below.

`keypressed(k)` and `keyreleased(k)` indicate whether key `k` was pressed/released since the last call to `cleareventflips()`. Be careful, here "pressed/released" should be interpreted as "indicated pressed/released by events read" not as a real-time state change.

_Example._ A game loop could test both the immediate state of some keys and state changes forother keys by using immediate functions after `cleareventflips()` followed by `clearevents()`.

```py
# Render game...

cleareventflips()
clearevents()

if keypressed(KEY_SHIFT):
    pass # Action !
if keydown(KEY_LEFT):
    player_x -= 1
if keydown(KEY_RIGHT):
    player_x += 1

# Simulate game...
```

## Basic rendering functions

Reference headers: [`<gint/display.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display.h), and for some details [`<gint/display-fx.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display-fx.h) and [`<gint/display-cg.h>`](https://gitea.planet-casio.com/Lephenixnoir/gint/src/branch/master/include/gint/display-cg.h).

TODO

## Image rendering functions

TODO

## Differences with gint's C API

- `dsubimage()` doesn't have its final parameter `int flags`. The flags are only minor optimizations and could be removed in future gint versions.
- Image constructors`image()` and `image_<format>()` don't exist in the C API.

TODO: There are more.
