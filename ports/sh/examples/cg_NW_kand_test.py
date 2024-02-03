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

getkey()
