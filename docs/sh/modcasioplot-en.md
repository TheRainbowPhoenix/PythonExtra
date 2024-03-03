# `casioplot`: CASIO's official I/O module

`casioplot` is a drawing library provided by CASIO as support for the official `turtle` and `matplotlib` modules. It was [announced in February 2020](https://www.planet-casio.com/Fr/forums/topic16154-1-modules-graphiques-python-en-avril-matplotlib-et-turtle.html) and [released in April](https://www.planet-casio.com/Fr/forums/topic16243-1-rendu-graphique-en-python-partie-1-decouverte-de-matplotlib-et-turtle.html). The module is notably available on G-III and fx-CG models with the same API. As of March 2024, `casioplot` is the only custom module in CASIO's official port of MicroPython. PythonExtra also provides the module for source compatibility.

```py
import casioplot
# or
from casioplot import *
```

**Contents**
- [Rendering functions](#rendering-functions)
- [Considerations for rendering](#considerations-for-rendering)
- [Differences with the official `casioplot` module](#differences-with-the-official-casioplot-module)

## Rendering functions

```py
show_screen() -> None
clear_screen() -> None
set_pixel(x: int, y: int, ?color: (int, int, int)) -> None
get_pixel(x: int, y: int) -> (int, int, int)
draw_string(x: int, y: int, text: str, ?color: (int, int, int), ?size: str) -> None
```

The rendering area is of size 128x64 on G-III and 384x216 on fx-CG. While the G-III supports only two colors (black and white), the fx-CG supports 65536 colors (16-bit RGB565). Still, the color format is _always_ (r,g,b) triplets with 0 ≤ r,g,b ≤ 255. This helps compatibility between models and with PC, at a performance cost. Calculators process these colors by _approximating_ them to the closest color they can represent.

Like the gint module, `casioplot` functions draw to an internal buffer called the VRAM, and only pushes that to the screen when explicitly asked to or at the end of the program.

`show_screen()` pushes the VRAM to screen after something has been drawn.

`clear_screen()` fills the VRAM in white.

`set_pixel()` changes the color of the pixel at (x,y) into the provided `color` encoded as an (r,g,b) triplet (black if not provided). Building triplets is a bit slow so don't write something like `set_pixel(x, y, (0,0,0))` in a loop, store the triplet in a variable and use that instead.

`get_pixel()` returns the color of the pixel at (x,y) in VRAM as an (r,g,b) triplet. Because VRAM stores colors in the same format as the display, which typically doesn't support all 16 million colors, `get_pixel()` immediately after `set_pixel()`usually returns an _approximation_ of the original color. Because it allocates a triplet, this function is also excruciatingly slow to call in a loop, to the point of creating GC-driven lag spikes that in some situations make it completely unusable.

`draw_string()` draws text. (x,y) is the location of the top-left corner of the rendered string. The text color is optional; if specified it should be an (r,g,b) triplet, otherwise it is black. The font size can be either of the strings `"small"`, `"medium"` or `"large"` and defaults to medium. On the G-III, the font sizes `"small"` and `"medium"` are identical, but on the fx-CG all three fonts are different. Any `'\n'` in the string is replaced with spaces when drawing.

TODO: Example

## Considerations for rendering

_Text mode._ `casioplot` automatically switches PythonExtra into graphics mode when imported. Programs that want to render graphics frames after a `print()` should `show_screen()` _before_ rendering the first frame since `print()` switches to text mode.

_Performance._ Writing to VRAM is the main reason why rendering in `casioplot` is sometimes much faster than in Basic. Basic pushes the rendered data to the display after every call, which is slow. Unfortunately, `casioplot` ends up losing for complex drawing tasks for which Basic has complex functions (like DrawStat) while `casioplot` just as single pixels. In this case, functions from the [gint module](modgint-en.md) can be of help.

## Differences with the official `casioplot` module

- PythonExtra's implementation doesn't automatically show the screen when the program finishes executing.
- The fonts for `draw_string()` are not identical on the G-III port.
- The fonts for `draw_string()` currently only support ASCII.
- The drawable space on fx-CG is at the top left section of the screen.
