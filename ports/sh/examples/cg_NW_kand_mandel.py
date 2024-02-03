# This script draws a Mandelbrot fractal set
# N_iteration: degree of precision
import kandinsky
import ion
import time

N_iteration = 10

for x in range(320):
  for y in range(222):
# Compute the mandelbrot sequence for the point c = (c_r, c_i) with start value z = (z_r, z_i)
    z = complex(0,0)
# Rescale to fit the drawing screen 320x222
    c = complex(3.5*x/319-2.5, -2.5*y/221+1.25)
    i = 0
    while (i < N_iteration) and abs(z) < 2:
      i = i + 1
      z = z*z+c
# Choose the color of the dot from the Mandelbrot sequence
    rgb = int(255*i/N_iteration)
    col = kandinsky.color(int(rgb),int(rgb*0.75),int(rgb*0.25))
# Draw a pixel colored in 'col' at position (x,y)
    kandinsky.set_pixel(x,y,col)

while not ion.keydown(ion.KEY_EXE):
  time.sleep(0.1)

