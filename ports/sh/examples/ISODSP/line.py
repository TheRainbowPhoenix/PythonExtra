from casioplot import *

def line(pos1,pos2):
  x1=int(pos1[0])
  y1=int(pos1[1])
  x2=int(pos2[0])
  y2=int(pos2[1])
  """if x2>x1:
    tmp=x1
    x1=x2
    x2=tmp
    del tmp
  if y2>y1:
    tmp=y1
    y1=y2
    y2=tmp
    del tmp
  dx=x1-x2
  dy=y1-y2
  if dx<dy:
    e=dx/dy
    d=x2
    for i in range(y2,y1):
      set_pixel(int(d),i)
      d+=e
  else:
    e=dy/dx
    d=y2
    for i in range(x2,x1):
      set_pixel(i,int(d))
      d+=e"""
  dx=abs(x2-x1)
  sx=x1<x2
  if sx==True:
    sx=1
  else:
    sx=-1
  dy=-abs(y2-y1)
  sy=y1<y2
  if sy==True:
    sy=1
  else:
    sy=-1
  error=dx+dy
  while True:
    set_pixel(x1,y1)
    if x1==x2 and y1==y2:
      break
    e2=2*error
    if e2>=dy:
      if x1==x2:
        break
      error=error+dy
      x1+=sx
    if e2<=dx:
      if y1==y2:
        break
      error=error+dx
      y1+=sy

def vramline(pos1,pos2,w,h,vram):
  x1=int(pos1[0])
  y1=int(pos1[1])
  x2=int(pos2[0])
  y2=int(pos2[1])
  """if x2>x1:
    tmp=x1
    x1=x2
    x2=tmp
    del tmp
  if y2>y1:
    tmp=y1
    y1=y2
    y2=tmp
    del tmp
  dx=x1-x2
  dy=y1-y2
  if dx<dy:
    e=dx/dy
    d=x2
    for i in range(y2,y1):
      vram[int(d)+i*w]='#'
      d+=e
  else:
    e=dy/dx
    d=y2
    for i in range(x2,x1):
      vram[i+int(d)*w]='#'
      d+=e"""
  dx=abs(x2-x1)
  sx=x1<x2
  if sx==True:
    sx=1
  else:
    sx=-1
  dy=-abs(y2-y1)
  sy=y1<y2
  if sy==True:
    sy=1
  else:
    sy=-1
  error=dx+dy
  while True:
    vram[x1+y1*w]='#'
    if x1==x2 and y1==y2:
      break
    e2=2*error
    if e2>=dy:
      if x1==x2:
        break
      error=error+dy
      x1+=sx
    if e2<=dx:
      if y1==y2:
        break
      error=error+dx
      y1+=sy
      return vram
