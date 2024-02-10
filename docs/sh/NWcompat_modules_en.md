# Using Numworks' modules `kandinsky`, `ion` and `time` with PythonExtra

PythonExtra offers the possibility of using certain Numworks modules in order to make the scripts of this machine compatible as is on Casio fx-CG50 (no support on the fx9860G due to insufficient memory and lack of color screen ).

This is a Work in Progress (WIP) and the support is subject to extensive testing at this stage. The port concerns the Numworks `kandinsky`, `ion` and `time` modules which are machine specific and are now supported via this implementation. The `math`, `cmath`, `random` modules being identical between the `Numworks` version and the `builtins` modules of MicroPython, they are therefore not part of this implementation but are perfectly usable without modification in the scripts.

Note: the `turtle` and `matplotlib.pyplot` modules are not included in this implementation. It is possible to use the `turtle`, `matplotlib` and `casioplot` modules from Casio Education which are perfectly functional and provided as an example in `ports/sh/examples`.


## `kandinsky`

The `kandinsky` module provides support for graphics primitives via high-performance `gint` routines. All the functions of this module are available:

- `color(r,g,b)`: Generates the value of the color r,g,b. You can also simply use a tuple to define a color: (r,g,b).

- `get_pixel(x,y)`: Returns the color of the pixel at x,y coordinates as a tuple (r,g,b).

- `set_pixel(x,y,color)`: Lights the pixel x,y of the color color.

- `draw_string(text,x,y,[color1],[color2])`: Displays the text at x,y coordinates. The arguments color1 (text color) and color2 (text background color) are optional.

- `fill_rect(x,y,width,height,color)`: Fills a rectangle of width width and height height with the color color at the point of x and y coordinates.

The module also offers a certain number of colors explicitly named and accessible by a character string. The following values ​​can be used instead of the color parameters of the `kandinsky` functions:
- "red", "r"
- "green", "g"
- "blue", "b"
- "black", "k"
- "white", "w"
- "yellow", "y"
- "pink"
- "magenta"
- "grey", "gray"
- "purple"
- "orange"
- "cyan"
- "brown"

The following functions are additions to take advantage of the wide screen of the fxCG and are therefore an extension of the `Kandinsky` module. They are therefore by definition not compatible with Python Numwork. These functions can be recognized by their names which all begin with `CGEXT_`:

- `CGEXT_Enable_Wide_Screen()`: Enables the fxCG wide screen, no settings are necessary. The x-coordinates of the physical screen can be negative to encroach on the left white stripe and greater than 319 pixels to encroach on the right white stripe;

- `CGEXT_Disable_Wide_Screen()`: Cancels the activation of the fxCG extended screen, no settings are necessary. The x coordinates of the physical screen will be constrained between 0 and 320 pixels. Beyond that, the route will not be carried out.

- `CGEXT_Is_Wide_Screen_Enabled()`: Returns `True` if the extended screen is active and `False` otherwise.

- `CGEXT_Set_Margin_Color( color )` : Paints the margin of the `Numworks` screen on fxCG with the specified color.

Note 1: after having made a plot in the extended area, it must be active to allow its deletion (typically via a call to the `fill_rect()` function with the appropriate parameters).

Note 2: In non-extended mode (by default when initializing the `Kandinsky` module) the screen coordinates go from (0,0) to (319,221) centered on the fxCG screen. In extended mode, the screen coordinates range from (-38,-1) to (358,223).


## `ion`

The `ion` module gives access to the `keydown(k)` function which returns True if the key k placed as an argument is pressed and False otherwise.

The “conversion” of the keys between the Numworks machine and Casio fxCG50 is done according to the following mapping:

| Numworks | Casio fxCG50 | Numworks Key # |
|----------|--------------|---------------------|
| KEY_LEFT     | KEY_LEFT | 0 |
| KEY_UP     | KEY_UP | 1 |
| KEY_DOWN     | KEY_DOWN | 2 |
| KEY_RIGHT     | KEY_RIGHT | 2 |
| KEY_OK      | KEY_F1 | 4 |
| KEY_BACK        | KEY_EXIT | 5 |
| KEY_HOME        | KEY_MENU | 6 |
| KEY_ONOFF       | KEY_ACON | 7 |
| ...           | ... | ... |
| KEY_SHIFT     | KEY_SHIFT | 12 |
| KEY_ALPHA     | KEY_ALPHA | 13 |
| KEY_XNT     | KEY_XOT | 14 |
| KEY_VAR     | KEY_VARS | 15 |
| KEY_TOOLBOX     | KEY_OPTN | 16 |
| KEY_BACKSPACE       | KEY_DEL | 17 |
| KEY_EXP     | KEY_EXP | 17 |
| KEY_LN     | KEY_LN | 19 |
| KEY_LOG     | KEY_LOG | 20 |
| KEY_IMAGINARY       | KEY_F2 | 21 |
| KEY_COMMA     | KEY_COMMA | 22 |
| KEY_POWER     | KEY_POWER | 23 |
| KEY_SINE        | KEY_SIN | 24 |
| KEY_COSINE      | KEY_COS | 25 |
| KEY_TANGENT     | KEY_TAN | 26 |
| KEY_PI      | KEY_F3 | 27 |
| KEY_SQRT        | KEY_F4 | 28 |
| KEY_SQUARE     | KEY_SQUARE | 29 |
| KEY_SEVEN       | KEY_7 | 30 |
| KEY_EIGHT       | KEY_8 | 31 |
| KEY_NINE        | KEY_9 | 32 |
| KEY_LEFTPARENTHESIS     | KEY_LEFTP | 33 |
| KEY_RIGHTPARENTHESIS        | KEY_RIGHTP | 34 |
| ...           | ... | ... |
| KEY_FOUR        | KEY_4 | 36  |
| KEY_FIVE        | KEY_5 | 37 |
| KEY_SIX     | KEY_6 | 38 |
| KEY_MULTIPLICATION      | KEY_MUL | 39 |
| KEY_DIVISION        | KEY_DIV | 40 |
| ...           | ... | ... |
| KEY_ONE     | KEY_1 | 42 |
| KEY_TWO     | KEY_2 | 43 |
| KEY_THREE       | KEY_3 | 44 |
| KEY_PLUS        | KEY_ADD | 45 |
| KEY_MINUS       | KEY_SUB | 46 |
| ...           | ... | ... |
| KEY_ZERO        | KEY_0 | 48 |
| KEY_DOT     | KEY_DOT | 49 |
| KEY_EE      | KEY_F5 | 50 |
| KEY_ANS     | KEY_NEG | 51 |
| KEY_EXE     | KEY_EXE | 52 |


## `time`

The `time` module gives access to two functions:

- `monotonic()`: Returns the clock value at the time the function is called.

- `sleep(t)`: Suspends execution for t seconds.
