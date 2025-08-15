.. _gint_quickref:

Quick reference for the Gint module
=====================================

Below is a quick reference for the gint module, which provides access to the calculator's screen and keyboard. If it is your first time working with this module, please read the main documentation first.

.. toctree::
   :maxdepth: 1

   ../../library/gint.rst

General Display Control
-----------------------

See :mod:`gint`. ::

    import gint

    # It's good practice to initialize the screen at the start
    gint.dclear(gint.C_WHITE)
    gint.dupdate()

    # Fill the screen with black
    gint.dclear(gint.C_BLACK)

    # Update the display to show the changes
    gint.dupdate()

Delay and timing
----------------

Use the :mod:`time <time>` module for delays. ::

    import time

    time.sleep_ms(500)      # wait for 500 milliseconds
    start = time.ticks_ms() # get value of millisecond counter
    delta = time.ticks_diff(time.ticks_ms(), start) # compute time difference

Keyboard Input
--------------

See :mod:`gint`. ::

    import gint

    # Wait (block) until a key is pressed
    event = gint.getkey()
    if event.key == gint.KEY_EXE:
        print("EXE key was pressed!")

    # Check if a key is currently held down (non-blocking)
    if gint.keydown(gint.KEY_RIGHT):
        # Move character right
        pass

    # Poll for events without blocking
    event = gint.pollevent()
    if event.type != gint.KEYEV_NONE:
        # An event occurred, process it
        print("Event type:", event.type)

Drawing Text
------------

See :func:`gint.dtext`. ::

    import gint

    # Draw "Hello!" in black at position (10, 20)
    gint.dtext(10, 20, gint.C_BLACK, "Hello!")

    # Draw centered text
    gint.dtext_opt(gint.DWIDTH // 2, 50, gint.C_BLUE, gint.C_NONE,
                 gint.DTEXT_CENTER, gint.DTEXT_MIDDLE, "Centered", -1)

    gint.dupdate()

Geometric Shapes
----------------

See :mod:`gint`. ::

    import gint

    # Draw a single red pixel
    gint.dpixel(5, 5, gint.C_RED)

    # Draw a green line
    gint.dline(0, 0, 100, 50, gint.C_GREEN)

    # Draw a blue filled rectangle
    gint.drect(20, 20, 80, 60, gint.C_BLUE)

    # Draw an empty circle with a black border
    gint.dcircle(150, 100, 30, gint.C_NONE, gint.C_BLACK)

    gint.dupdate()

Image Handling
--------------

See :class:`gint.image`. ::

    import gint

    # Assume `my_sprite` is an `image` object loaded previously.
    # fxconv can be used to generate image objects from PNG files.
    # my_sprite = gint.image(gint.IMAGE_P4_RGB565A, ...)

    # Draw the entire image at position (50, 50)
    gint.dimage(50, 50, my_sprite)

    # Draw a 16x16 portion of the image (e.g. from a spritesheet)
    # This draws the sub-image from (32,0) in the source image
    # to the screen at (100, 100).
    gint.dsubimage(100, 100, my_sprite, 32, 0, 16, 16)

    gint.dupdate()