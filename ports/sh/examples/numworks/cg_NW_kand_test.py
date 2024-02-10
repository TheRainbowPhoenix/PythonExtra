from kandinsky import *
from gint import *

draw_string( "Hello Kandinsky", 10, 10, "white", "red" )

draw_string( "Hello Kandinsky", 10, 200, "k" )

fill_rect( 25, 25, 100, 100, 00000 )
fill_rect( 60, 25, 10, 100, 65000 )
fill_rect( 25, 60, 100, 10, 00031 ) 

fill_rect( 100, 100, 25, 25, "green" )

fill_rect( 200, 100, 25, 25, (255,255,0) )

fill_rect( 200, 50, 25, 25, (128,0,255) )

set_pixel( 150, 150, "red" )
set_pixel( 160, 160, (0,0,255) )


# The following functions are only valid on fxCG (this is an extension of Kandinsky to take benefit of wide screen

color = "black"

if CGEXT_Is_Wide_Screen_Enabled() :
  color = "red"
fill_rect( -100, 150, 500, 10, color )

CGEXT_Enable_Wide_Screen()

if CGEXT_Is_Wide_Screen_Enabled() :
  color = "green"
else :
  color = "purple"
fill_rect( -100, 165, 500, 10, color )

CGEXT_Disable_Wide_Screen()

if CGEXT_Is_Wide_Screen_Enabled() :
  color = "blue"
else :
  color = "red"
fill_rect( -100, 180, 500, 10, color )

getkey()
