import casioplot
from math import sqrt
from math import pi
from math import cos
from math import sin
from math import atan2

turtleshapes={"classic":[[-9,5],[-9,4],[-8,4],[-8,3],[-8,2],[-8,-2],[-8,-3],[-8,-4],[-9,-4],[-9,-5],[-7,4],[-7,1],[-7,0],[-7,-1],[-7,-4],[-6,3],[-6,-3],[-5,3],[-5,-3],[-4,2],[-4,-2],[-3,2],[-3,-2],[-2,1],[-2,-1],[-1,1],[-1,-1],[0,0]],"turtle": [[-3,3],[2,3],[-2,2],[-1,2],[0,2],[1,2],[-2,1],[-1,1],[1,1],[0,1],[2,1],[3,1],[-2,0],[-1,0],[0,0],[1,0],[-1,-1],[-2,-1],[0,-1],[1,-1],[2,0],[3,0],[-3,-2],[2,-2]]}
turtle_name="classic"
turtle_data=turtleshapes[turtle_name]
turtle_pos=[0,0]
turtle_angle=0
turtle_color=(0,0,0)
writing=True
pen_size=1
pen_pixels=[[0,0]]
turtle_buffer=[]
turtle_speed=5
frame_count=0
turtle_visible=True

def _draw_turtle(x,y,a,c):
  global turtle_buffer
  def inbuffer(x,y):
    inlist=False
    for i in range(len(turtle_buffer)):
      if x==turtle_buffer[i][0] and y==turtle_buffer[i][1]:
        inlist=True
    return inlist
  if turtle_visible==True:
    u=cos(a*pi/180)
    v=sin(a*pi/180)
    for point in turtle_data:
      xx=x+(point[0]*u-point[1]*v)
      yy=y+(point[1]*u+point[0]*v)
      xpixel=int(round(xx+64))
      ypixel=int(round(-yy+32))
      if (0<=xpixel<=127 and 0<=ypixel<=63):
        if not inbuffer(xpixel,ypixel):
          turtle_buffer+=[[xpixel,ypixel,casioplot.get_pixel(xpixel,ypixel)]]
        casioplot.set_pixel(xpixel,ypixel,c)

def _erase_turtle():
  global turtle_buffer
  for i in range(len(turtle_buffer)):
    xpixel=turtle_buffer[i][0]
    ypixel=turtle_buffer[i][1]
    if turtle_buffer[i][2]!=None :
      lastcolor=turtle_buffer[i][2]
    else:
      lastcolor=(255,255,255)
    casioplot.set_pixel(xpixel,ypixel,lastcolor)
  turtle_buffer.clear()

def _pen_brush(x,y,turtle_color):
  global frame_count
  _erase_turtle()
  xpixel=int(round(x+64))
  ypixel=int(round(-y+32))
  if writing==True and (0<=xpixel<=127 and 0<=ypixel<=63) :
    colorpixel=(int(turtle_color[0]*255), int(turtle_color[1]*255),int(turtle_color[2]*255))

    for point in pen_pixels:
      casioplot.set_pixel(xpixel+point[0],ypixel+point[1],colorpixel)

    frame_count+=1
    if turtle_speed!=0:
      if frame_count%(turtle_speed*4)==0:
        _draw_turtle(x,y,turtle_angle,colorpixel)
        casioplot.show_screen()
    else :
      if frame_count%500==0:
        _draw_turtle(x,y,turtle_angle,colorpixel)
        casioplot.show_screen()

def _refresh_turtle():
  c=(int(turtle_color[0]*255), int(turtle_color[1]*255),int(turtle_color[2]*255))
  _erase_turtle()
  _draw_turtle(turtle_pos[0],turtle_pos[1],turtle_angle,c)
  casioplot.show_screen()

def _conv_angle(a):
  a=a%360
  if a < 0:
    a=360+a
  return a

def back(n):
  forward(-n)

def backward(n):
  back(n)

def bk(n):
  backward(n)

def circle(radius,extent=360):
  global  turtle_angle, turtle_pos
  x1=turtle_pos[0]
  y1=turtle_pos[1]
  if round(radius)==0:
    _pen_brush(x1,y1,turtle_color)
    turtle_angle+=extent
  elif round(extent,8)==0:
    _pen_brush(x1,y1,turtle_color)
  else:
    e=radius/abs(radius)
    theta=extent*pi/180*e
    Rx=cos(theta)
    Ry=sin(theta)
    Dx=radius*sin(turtle_angle*pi/180)
    Dy=-radius*cos(turtle_angle*pi/180)
    xcenter=x1-Dx
    ycenter=y1-Dy
    nbpixelarc=int(round(abs(radius*theta*1.05)))
    angle=turtle_angle
    if nbpixelarc!=0:
      alpha=theta/nbpixelarc
      for k in range(nbpixelarc+1):
        x=xcenter+Dx*cos(alpha*k)-Dy*sin(alpha*k)
        y=ycenter+Dx*sin(alpha*k)+Dy*cos(alpha*k)
        turtle_angle+=alpha*180/pi
        _pen_brush(x,y,turtle_color)
    turtle_pos[0]=xcenter+Dx*Rx-Dy*Ry
    turtle_pos[1]=ycenter+Dx*Ry+Dy*Rx
    turtle_angle=angle+extent*e
  turtle_angle=_conv_angle(turtle_angle)
  _refresh_turtle()

def clear():
  _erase_turtle()
  casioplot.clear_screen()
  casioplot.show_screen()
  _refresh_turtle()

def distance(x,y):
  return sqrt((x-turtle_pos[0])**2+(y-turtle_pos[1])**2)

def down():
  global writing
  writing=True

def fd(d):
  forward(d)

def forward(d):
  global turtle_pos
  dx=d*cos(turtle_angle*pi/180)
  dy=d*sin(turtle_angle*pi/180)
  x1=turtle_pos[0]
  y1=turtle_pos[1]
  if round(abs(d))==0:
    _pen_brush(x1+dx,y1+dy,turtle_color)
  elif abs(dx)>=abs(dy):
    e=int(dx/abs(dx))
    m=dy/dx
    p=y1-m*x1
    for x in range(int(round(x1)),int(round(x1+dx)),e):
      _pen_brush(x,m*x+p,turtle_color)
  else:
    e=int(dy/abs(dy))
    m=dx/dy
    p=x1-m*y1
    for y in range(int(round(y1)),int(round(y1+dy)),e):
      _pen_brush(m*y+p,y,turtle_color)
  turtle_pos[0]+=dx
  turtle_pos[1]+=dy
  _refresh_turtle()

def goto(x,y):
  a=turtle_angle

  setheading(towards(x,y))
  forward(distance(x,y))
  setheading(a)

  _refresh_turtle()

def heading():
  return turtle_angle

def hideturtle():
  global turtle_visible
  turtle_visible=False
  _refresh_turtle()

def home():
  global turtle_angle
  goto(0,0)
  turtle_angle=0
  _refresh_turtle()

def ht():
  hideturtle()

def isdown():
  return writing

def isvisible():
  return turtle_visible

def left(a):
  right(-a)

def lt(a):
  left(a)

def pd():
  down()

def pencolor(*c):
  global turtle_color
  colornames={"black":(0,0,0),"blue":(0,0,1),"green":(0,1,0),"red":(1,0,0),"cyan":(0,1,1),"yellow":(1,1,0),"magenta":(1,0,1),"white":(1,1,1),"orange":(1,0.65,0),"purple":(0.66,0,0.66),"brown":(0.75,0.25,0.25),"pink":(1,0.75,0.8),"grey":(0.66,0.66,0.66)}
  if c==():
    return turtle_color
  elif c[0] in colornames:
    turtle_color=colornames[c[0]]
  elif isinstance(c[0],(list,tuple)) and len(c[0])==3 and isinstance(c[0][0],(int,float)) and isinstance(c[0][1],(int,float)) and isinstance(c[0][2],(int,float)) and 0<=c[0][0]<=1 and 0<=c[0][1]<=1 and 0<=c[0][2]<=1:
    turtle_color=list(c[0])

  elif len(c)==3 and isinstance(c[0],(int,float)) and isinstance(c[1],(int,float)) and isinstance(c[2],(int,float)) and 0<=c[0]<=1 and 0<=c[1]<=1 and 0<=c[2]<=1:
    turtle_color=list(c)

  else:
    raise ValueError('error using pencolor : enter a color text or 3 floats between 0 and 1')
  _refresh_turtle()

def pendown():
  down()

def pensize(n=None):
  global pen_pixels,pen_size
  penshape=[[0,0],[1,0],[0,1],[-1,0],[0,-1],[1,1],[1,-1],[-1,1],[-1,-1],[2,0],[0,2],[-2,0],[0,-2],[2,1],[1,2],[-2,1],[-1,2],[2,-1],[1,-2],[-2,-1],[-1,-2]]
  if n==None:
    return pen_size
  elif isinstance(n,(int,float)) and n>=0:
    pen_size=n
    if round(n)==0 or round(n)==1 :
      pen_pixels=[[0,0]]
    elif round(n)==2 :
      pen_pixels=penshape[0:5]
    elif round(n)==3 :
      pen_pixels=penshape[0:9]
    elif round(n)==4 :
      pen_pixels=penshape[0:13]
    elif round(n)==5 :
      pen_pixels=penshape[0:21]
    elif round(n)>5 :
      pen_pixels=penshape[0:21]
      pen_size=5
      print('Userwarning: pensize over 5 automatically set to 5.')
  else:
    raise ValueError('Error using function pensize: enter a real between 0 & 5')
  _refresh_turtle()

def penup():
  global writing
  writing=False

def pos():
  return position()

def position():
  return (xcor(),ycor())

def pu():
  penup()

def reset():
  global turtle_color,writing,pen_pixels,turtle_speed,turtle_visible,pen_size
  turtle_color=(0,0,0)
  clear()
  hideturtle()
  penup()
  home()
  pendown()
  writing=True
  pen_size=1
  pen_pixels=[[0,0]]
  turtle_speed=5
  shape("classic")
  turtle_visible=True
  _refresh_turtle()

def right(a):
  global turtle_angle
  if isinstance(a, (int, float)):
    turtle_angle = _conv_angle(turtle_angle-a)
  else:
    raise ValueError('error')
  _refresh_turtle()

def rt(a):
  right(a)

def seth(a):
  setheading(a)

def setheading(a):
  global turtle_angle
  turtle_angle=_conv_angle(a)
  _refresh_turtle()

def setpos(x,y):
  goto(x,y)

def setposition(x,y):
  setpos(x,y)

def setx(x):
  goto(x,turtle_pos[1])

def sety(y):
  goto(turtle_pos[0],y)

def shape(name=None):
  global turtle_name,turtle_data
  if name==None:
    return turtle_name
  elif name in turtleshapes:
    turtle_name=name
    turtle_data=turtleshapes[name]
  else:
    raise ValueError('available shapes: "classic" or "turtle"')
  _refresh_turtle()

def showturtle():
  global turtle_visible
  turtle_visible=True
  _refresh_turtle()

def speed(speed=None):
  global turtle_speed
  speedwords = {'fastest':0, 'fast':10, 'normal':6, 'slow':3, 'slowest':1 }
  if speed==None:
    return turtle_speed
  elif isinstance(speed,(int,float)) and (speed<=0.5 or speed>=10.5):
    turtle_speed=0
  elif isinstance(speed,(int,float)) and (0.5<speed<10.5):
    turtle_speed=int(round(speed))
  elif isinstance(speed,str) and speed in speedwords:
    turtle_speed=speedwords[speed]
  else:
    raise ValueError("Error using function speed: enter a real between 0 & 10")

def st():
  showturtle()

def towards(x,y):
  if round(x-turtle_pos[0],8)==0 and round(y-turtle_pos[1],8)==0:
    return 0
  else:
    ang=atan2(y-turtle_pos[1],x-turtle_pos[0])*180/pi
    if ang>=0:
        return (ang)
    else:
        return (360+ang)

def up():
  penup()

def width(n=None):
  return pensize(n)

def write(text):
  _refresh_turtle()
  xpixel=int(round(turtle_pos[0]+64))
  ypixel=int(round(-turtle_pos[1]+32))
  c=(int(turtle_color[0]*255), int(turtle_color[1]*255),int(turtle_color[2]*255))
  casioplot.draw_string(xpixel,ypixel,str(text),c,"small")
  casioplot.show_screen()

def xcor():
  return round(turtle_pos[0],6)
def ycor():
  return round(turtle_pos[1],6)
