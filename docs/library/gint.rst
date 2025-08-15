:mod:`gint` --- CASIO calculator hardware access
=================================================

.. module:: gint
    :synopsis: functions for graphics, keyboard, and timers.

The gint module provides a high-level interface to the gint library, allowing direct access to the calculator's hardware features like the display and keyboard.

Classes
-------

.. class:: KeyEvent

    A data object representing a keyboard event. This object is returned by functions like :func:`pollevent` and :func:`getkey`. It contains the following read-only attributes:

    .. attribute:: time

        Timestamp of the event.

    .. attribute:: mod
    .. attribute:: shift
    .. attribute:: alpha

        Booleans indicating the state of modifier keys.

    .. attribute:: type

        The type of event, such as ``KEYEV_DOWN`` or ``KEYEV_UP``.

    .. attribute:: key

        The key code of the key involved in the event (e.g., ``KEY_EXE``).

    .. attribute:: x
    .. attribute:: y

        For touch events, the coordinates of the touch.

.. class:: image

    Represents a graphical image stored in VRAM. It is designed to handle various pixel formats.

    On black-and-white models, it supports 2-5 colors through different profiles, with data stored in a packed row-major format (32 pixels per 4-byte word).

    .. method:: __init__(profile, color_count, width, height, stride, data, palette)

        Constructs an image from pixel data and palette information.

        :param profile: Image format constant (e.g., ``IMAGE_MONO``, ``IMAGE_RGB565A``).
        :param color_count: Number of colors in the palette.
        :param width: Image width in pixels.
        :param height: Image height in pixels.
        :param stride: Number of bytes per row of pixel data.
        :param data: A buffer-like object containing the raw pixel data.
        :param palette: A buffer-like object containing the color palette data.

    .. attribute:: format

        The image format, represented by an ``IMAGE_*`` constant.

    .. attribute:: flags
    .. attribute:: color_count

        Number of colors in the image's palette.

    .. attribute:: width
    .. attribute:: height

        The dimensions of the image in pixels.

    .. attribute:: stride

        The number of bytes per row in the image data.

    .. attribute:: data

        A buffer-like object holding the raw pixel data.

    .. attribute:: palette

        A buffer-like object holding the color palette.

.. class:: GintFont

    Represents a custom font that can be used for drawing text. Created by the :func:`font` constructor.

    .. attribute:: prop
    .. attribute:: line_height
    .. attribute:: data_height
    .. attribute:: block_count
    .. attribute:: glyph_count
    .. attribute:: char_spacing
    .. attribute:: line_distance
    .. attribute:: blocks
    .. attribute:: data
    .. attribute:: width
    .. attribute:: storage_size
    .. attribute:: glyph_index
    .. attribute:: glyph_width

Functions
---------

Keyboard Functions
~~~~~~~~~~~~~~~~~~

.. function:: clearevents()

    Clears all pending keyboard events from the event queue. This is useful when you need to check the immediate keyboard state with :func:`keydown` without being influenced by historical events. It is equivalent to running 
    
    .. code-block:: python
    
        while pollevent().type != KEYEV_NONE: pass
    

.. function:: pollevent() -> KeyEvent

    Retrieves the oldest unread keyboard event from the queue. This function is non-blocking.

    :return: A :class:`KeyEvent` object describing the event. If the queue is empty, it returns a dummy event where `type` is ``KEYEV_NONE``.

    Example:

    .. code-block:: python

        # Process all queued events
        while True:
            ev = pollevent()
            if ev.type == KEYEV_NONE:
                break
            # Handle the event here

.. function:: keydown(key) -> bool

    Checks if a specific key is currently held down.

    :param key: The key to check, using a ``KEY_*`` constant.
    :return: ``True`` if the key is pressed, ``False`` otherwise.

    .. note:: This function requires that the event queue has been recently processed by calling :func:`clearevents` or :func:`pollevent`.

.. function:: keydown_all(keys) -> bool

    Checks if all specified keys are currently held down simultaneously.

    :param keys: A list of ``KEY_*`` constants.
    :return: ``True`` if all keys are pressed, ``False`` otherwise.

.. function:: keydown_any(keys) -> bool

    Checks if any of the specified keys are currently held down.

    :param keys: A list of ``KEY_*`` constants.
    :return: ``True`` if at least one key is pressed, ``False`` otherwise.

.. function:: cleareventflips()

    Resets the tracking state for key presses and releases. This should be called before using :func:`keypressed` or :func:`keyreleased` in a loop (e.g., at the start of each frame in a game).

.. function:: keypressed(key) -> bool

    Checks if a key was pressed since the last call to :func:`cleareventflips`.

    :param key: The ``KEY_*`` constant to check.
    :return: ``True`` if the key transitioned from a released to a pressed state.

.. function:: keyreleased(key) -> bool

    Checks if a key was released since the last call to :func:`cleareventflips`.

    :param key: The ``KEY_*`` constant to check.
    :return: ``True`` if the key transitioned from a pressed to a released state.

.. function:: getkey() -> KeyEvent

    Waits (blocks execution) for a key press or repeat event and returns it. System actions like pressing the MENU key may exit the program.

    :return: A :class:`KeyEvent` object with details of the key press, including shift/alpha modifiers.

.. function:: getkey_opt(options, timeout_ms=None) -> KeyEvent

    A configurable, non-blocking version of :func:`getkey` with a timeout.

    :param options: A combination of ``GETKEY_*`` flags (e.g., ``GETKEY_MOD_SHIFT | GETKEY_REP_ARROWS``).
    :param timeout_ms: The maximum time to wait in milliseconds. If ``None``, it waits indefinitely.
    :return: A :class:`KeyEvent` object, or a dummy event with ``type=KEYEV_NONE`` if the timeout is reached.

.. function:: keycode_function(keycode) -> int

    Gets the F-key number (1-6) associated with a key code.

    :param keycode: A key code constant (e.g., ``KEY_F1``).
    :return: The F-key number, or -1 if the key is not an F-key.

.. function:: keycode_digit(keycode) -> int

    Gets the digit (0-9) associated with a numeric key code.

    :param keycode: A key code constant (e.g., ``KEY_0``).
    :return: The digit value, or -1 if the key is not a digit key.

Display Functions
~~~~~~~~~~~~~~~~~

.. function:: C_RGB(r, g, b) -> int

    Creates a color value from RGB components.

    .. note:: This function is only available on color display models.

    :param r: Red component (0-31).
    :param g: Green component (0-31).
    :param b: Blue component (0-31).
    :return: An integer representing the color.

.. function:: dclear(color)

    Fills the entire screen with a specified color.

.. function:: dupdate()

    Refreshes the display to show all changes made to the VRAM. This must be called after drawing operations for them to become visible.

.. function:: dpixel(x, y, color)

    Draws a single pixel at the specified coordinates.

.. function:: dgetpixel(x, y) -> int

    Gets the color of the pixel at the specified coordinates from VRAM.

    :return: The color value, or -1 if the coordinates are out of bounds.

.. function:: drect(x1, y1, x2, y2, color)

    Draws a filled rectangle. The coordinates (x1, y1) and (x2, y2) can be any two opposite corners.

.. function:: drect_border(x1, y1, x2, y2, fill_color, border_width, border_color)

    Draws a filled rectangle with an inner border.

.. function:: dline(x1, y1, x2, y2, color)

    Draws a straight line between two points.

.. function:: dhline(y, color)

    Draws a horizontal line across the entire width of the screen at a given y-coordinate.

.. function:: dvline(x, color)

    Draws a vertical line across the entire height of the screen at a given x-coordinate.

.. function:: dcircle(x, y, r, fill, border)

    Draws a circle with a specified fill and border color. Use ``C_NONE`` for a transparent fill or border.

.. function:: dellipse(x1, y1, x2, y2, fill, border)

    Draws an ellipse that fits within the specified bounding box.

.. function:: dpoly(vertices, fill, border)

    Draws a polygon. The polygon is automatically closed if the first and last vertices are not the same.

    :param vertices: A flat list of coordinates, e.g., ``[x0, y0, x1, y1, ...]``.
    :param fill: The fill color (use ``C_NONE`` for transparent).
    :param border: The border color (use ``C_NONE`` for no border).

.. function:: dtext(x, y, fg, text)

    Draws a string of text at the specified coordinates.

    :param x: Left starting position in pixels.
    :param y: Top starting position in pixels.
    :param fg: The text color.
    :param text: The string to display.

.. function:: dtext_opt(x, y, fg, bg, halign, valign, text, size)

    Draws text with advanced options for alignment, background, and wrapping.

    :param x: Reference X coordinate.
    :param y: Reference Y coordinate.
    :param fg: Text color.
    :param bg: Background color (``C_NONE`` for transparent).
    :param halign: Horizontal alignment (``DTEXT_LEFT``, ``DTEXT_CENTER``, ``DTEXT_RIGHT``).
    :param valign: Vertical alignment (``DTEXT_TOP``, ``DTEXT_MIDDLE``, ``DTEXT_BOTTOM``).
    :param text: The string to display.
    :param size: Maximum width for wrapping; -1 to disable.

.. function:: dimage(x, y, img)

    Draws a complete :class:`image` object at the specified coordinates.

.. function:: dsubimage(x, y, img, left, top, width, height)

    Draws a specific subregion of an :class:`image` object at the specified coordinates.

Image Constructors
~~~~~~~~~~~~~~~~~~

.. note:: The following functions are used to create :class:`image` objects for color displays.

.. function:: image_rgb565(width, height, data) -> image
.. function:: image_rgb565a(width, height, data) -> image
.. function:: image_p8_rgb565(width, height, data, palette) -> image
.. function:: image_p8_rgb565a(width, height, data, palette) -> image
.. function:: image_p4_rgb565(width, height, data, palette) -> image
.. function:: image_p4_rgb565a(width, height, data, palette) -> image

Font Constructor
~~~~~~~~~~~~~~~~

.. function:: font(...) -> GintFont

    Constructs a custom ``GintFont`` object. Takes numerous parameters describing the font's properties and glyph data, such as ``line_height``, ``char_spacing``, ``blocks``, and ``data``.

Constants
---------

Keyboard Constants
~~~~~~~~~~~~~~~~~~

*   **Key Codes**: ``KEY_F1``, ``KEY_F2``, ``KEY_F3``, ``KEY_F4``, ``KEY_F5``, ``KEY_F6``, ``KEY_SHIFT``, ``KEY_OPTN``, ``KEY_VARS``, ``KEY_MENU``, ``KEY_LEFT``, ``KEY_UP``, ``KEY_ALPHA``, ``KEY_SQUARE``, ``KEY_POWER``, ``KEY_EXIT``, ``KEY_DOWN``, ``KEY_RIGHT``, ``KEY_XOT``, ``KEY_LOG``, ``KEY_LN``, ``KEY_SIN``, ``KEY_COS``, ``KEY_TAN``, ``KEY_FRAC``, ``KEY_FD``, ``KEY_LEFTP``, ``KEY_RIGHTP``, ``KEY_COMMA``, ``KEY_ARROW``, ``KEY_7``, ``KEY_8``, ``KEY_9``, ``KEY_DEL``, ``KEY_4``, ``KEY_5``, ``KEY_6``, ``KEY_MUL``, ``KEY_DIV``, ``KEY_1``, ``KEY_2``, ``KEY_3``, ``KEY_ADD``, ``KEY_SUB``, ``KEY_0``, ``KEY_DOT``, ``KEY_EXP``, ``KEY_NEG``, ``KEY_EXE``, ``KEY_ACON``, ``KEY_HELP``.
*   **Key Event Types**: ``KEYEV_NONE``, ``KEYEV_DOWN``, ``KEYEV_UP``, ``KEYEV_HOLD``, ``KEYEV_TOUCH_DOWN``, ``KEYEV_TOUCH_UP``, ``KEYEV_TOUCH_DRAG``.
*   **getkey() Options**: ``GETKEY_MOD_SHIFT``, ``GETKEY_MOD_ALPHA``, ``GETKEY_BACKLIGHT``, ``GETKEY_MENU``, ``GETKEY_REP_ARROWS``, ``GETKEY_REP_ALL``, ``GETKEY_REP_PROFILE``, ``GETKEY_FEATURES``, ``GETKEY_NONE``, ``GETKEY_DEFAULT``.

Display Constants
~~~~~~~~~~~~~~~~~

*   **Screen Dimensions**: ``DWIDTH``, ``DHEIGHT``.
*   **Text Alignment**: ``DTEXT_LEFT``, ``DTEXT_CENTER``, ``DTEXT_RIGHT``, ``DTEXT_TOP``, ``DTEXT_MIDDLE``, ``DTEXT_BOTTOM``.
*   **Colors**: ``C_WHITE``, ``C_LIGHT``, ``C_DARK``, ``C_BLACK``, ``C_INVERT``, ``C_NONE``, ``C_LIGHTEN``, ``C_DARKEN``, ``C_RED``, ``C_GREEN``, ``C_BLUE``.
*   **Image Formats**: ``IMAGE_MONO``, ``IMAGE_MONO_ALPHA``, ``IMAGE_GRAY``, ``IMAGE_GRAY_ALPHA``, ``IMAGE_RGB565``, ``IMAGE_RGB565A``, ``IMAGE_P8_RGB565``, ``IMAGE_P8_RGB565A``, ``IMAGE_P4_RGB565``, ``IMAGE_P4_RGB565A``.
*   **Image Flags**: ``IMAGE_FLAGS_DATA_RO``, ``IMAGE_FLAGS_PALETTE_RO``, ``IMAGE_FLAGS_DATA_ALLOC``, ``IMAGE_FLAGS_PALETTE_ALLOC``.