:mod:`casioplot` --- simplified graphics plotting
=====================================================

.. module:: casioplot
   :synopsis: a simple interface for drawing graphics and text.

The casioplot module provides a set of basic, easy-to-use functions for drawing on the calculator's screen. It is designed for beginners and simple graphical applications.

Functions
---------

.. function:: show_screen()

   Updates the physical display with the content that has been drawn in the video memory (VRAM). This function must be called to make any drawings or text visible.

.. function:: clear_screen()

   Fills the entire screen with white, effectively erasing all existing content from the drawing buffer.

.. function:: set_pixel(x, y, color=(0, 0, 0))

   Sets the color of a single pixel at a specified (x, y) coordinate.

   :param x: The horizontal position of the pixel.
   :param y: The vertical position of the pixel.
   :param color: An RGB tuple where each value is between 0 and 255. Defaults to black.

   .. note::
      For better performance, especially in loops, it is recommended to store color tuples in variables instead of creating them repeatedly inside the loop.

   Example:

   .. code-block:: python

      # Define colors
      RED = (255, 0, 0)
      BLACK = (0, 0, 0)

      # Set a red pixel at (10, 10)
      set_pixel(10, 10, RED)

      # Set a black pixel at (11, 10) using the default color
      set_pixel(11, 10)

.. function:: get_pixel(x, y) -> tuple

   Retrieves the color of the pixel at a specified (x, y) coordinate.

   :param x: The horizontal position of the pixel.
   :param y: The vertical position of the pixel.
   :return: An RGB tuple (r, g, b) representing the color of the pixel.

   .. admonition:: Performance Warning
      :class: attention

      This function can be slow when called frequently inside a loop due to the creation of a new tuple for each call.

   Example:

   .. code-block:: python

      # Get the color of the pixel at (10, 10)
      pixel_color = get_pixel(10, 10)
      print("Color is:", pixel_color)

.. function:: draw_string(x, y, text, color=(0, 0, 0), size="medium")

   Draws a string of text on the screen with its top-left corner at the specified (x, y) coordinate.

   :param x: The horizontal position for the top-left corner of the text.
   :param y: The vertical position for the top-left corner of the text.
   :param text: The string of text to be drawn. Newline characters (\\n) in the string will be rendered as spaces.
   :param color: An RGB tuple (r, g, b) for the text color. Defaults to black.
   :param size: The font size. Can be one of ``"small"``, ``"medium"``, or ``"large"``. Defaults to "medium".

   Example:

   .. code-block:: python

      # Define a blue color
      BLUE = (0, 0, 255)

      # Draw "Hello, World!" in large blue text at (0, 0)
      draw_string(0, 0, "Hello, World!", BLUE, "large")

      # Draw smaller text in black
      draw_string(0, 20, "This is a test.", size="small")
