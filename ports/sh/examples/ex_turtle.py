import turtle
from math import exp

def try_colormode(m):
  try: turtle.colormode(m)
  except: pass
def try_pencolor(c):
  try: turtle.pencolor(c)
  except: pass
  try: turtle.color(c)
  except: pass

try_colormode(1)
turtle.penup()
turtle.goto(0, -20)
turtle.pendown()
for i in range(1,37):
  red=(exp(-0.5 * ((i-6)/12)**2))
  green=(exp(-0.5 * ((i-18)/12)**2))
  blue=(exp(-0.5 * ((i-30)/12)**2))
  try_pencolor([red,green,blue])
  turtle.circle(50-i)
  turtle.right(10)
