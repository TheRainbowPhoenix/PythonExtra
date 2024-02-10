from kandinsky import *
from math import *
import ion
import time

def cercle1(x0,y0,r,c,e):
  for i in range(2*e):
    xd=x0-int((r-i*0.5)/sqrt(2))
    xf=x0+int((r-i*0.5)/sqrt(2))
    for x in range(xd,xf+1):
      x1=x
      y1=y0+int(sqrt((r-i*0.5)**2-(x-x0)**2))
      if sqrt((160-x1)**2+(111-y1)**2)<r:
        set_pixel(x1,y1,c)
      for j in range(3):
        x2=x0+y1-y0
        y2=y0+x0-x1
        if sqrt((160-x2)**2+(111-y2)**2)<r:
          set_pixel(x2,y2,c)
        x1,y1=x2,y2

def cercle2(x0,y0,r,c,e):
  for i in range(2*e):
    xd=x0-int((r-i*0.5)/sqrt(2))
    xf=x0+int((r-i*0.5)/sqrt(2))
    for x in range(xd,xf+1):
      x1=x
      y1=y0+int(sqrt((r-i*0.5)**2-(x-x0)**2))
      set_pixel(x,y1,c)
      for j in range(3):
        x2=x0+y1-y0
        y2=y0+x0-x1
        set_pixel(x2,y2,c)
        x1,y1=x2,y2

def rosace1(n,r,c,e):
  x,y=160+r,111
  for i in range(n):
    x1=int(160+r*cos(i*2*pi/n))
    y1=int(111+r*sin(i*2*pi/n))
    cercle1(x1,y1,r,c,e)
  cercle1(160,111,r,c,e)
    
def rosace2(n,r,c,e):
  x,y=160+r,111
  for i in range(n):
    x1=int(160+r*cos(i*2*pi/n))
    y1=int(111+r*sin(i*2*pi/n))
    cercle2(x1,y1,r,c,e)
    
def rosace3(n,r,c1,c2,c3,e):
  for k in range(2):
    rj=r
    for j in range(n-2):
      rj=int(rj-rj/(2*n))
      if k==0:rj+=1
      x,y=160+rj,111  
      for i in range(n):
        x1=int(160+rj*cos(i*2*pi/n))
        y1=int(111+rj*sin(i*2*pi/n))
        if k==0:
          col=c3
        else:
          if j==0 or j>n-4:col=c2
          else:col=c1
        cercle2(x1,y1,rj,col,e)

      
col1=color(5,50,120)
col2=color(255,45,45) 
col3=color(245,225,25)    
#rosace1(12,50,col1,2)
#rosace2(12,50,col1,1)
rosace3(10,55,col1,col2,col3,1)

while not ion.keydown(ion.KEY_EXE):
  time.sleep(0.1)