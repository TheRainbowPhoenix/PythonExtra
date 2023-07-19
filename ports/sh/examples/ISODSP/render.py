from line import *
from data2 import *

"""
BLOCK_H=12
BLOCK_W=32
"""
BLOCK_H=18
BLOCK_W=18

BLOCK_13=BLOCK_H//3
BLOCK_23=BLOCK_13*2

BLOCK_W12=BLOCK_W//2

BLOCK_H131=BLOCK_13-1
BLOCK_W121=BLOCK_W12-1

block = [
  [[0,BLOCK_13-1],[BLOCK_W12-1,0]],
  [[0,BLOCK_13-1],[BLOCK_W12-1,BLOCK_23-1]],
  [[BLOCK_W12-1,0],[BLOCK_W-1,BLOCK_13-1]],
  [[BLOCK_W12-1,BLOCK_23-1],[BLOCK_W-1,BLOCK_13-1]],
  [[0,BLOCK_13-1],[0,BLOCK_23-1]],
  [[BLOCK_W12-1,BLOCK_23-1],[BLOCK_W12-1,BLOCK_H-1]],
  [[BLOCK_W-1,BLOCK_13-1],[BLOCK_W-1,BLOCK_23-1]],
  [[0,BLOCK_23-1],[BLOCK_W12-1,BLOCK_H-1]],
  [[BLOCK_W12-1,BLOCK_H-1],[BLOCK_W-1,BLOCK_23-1]],
]

def genblock(block):
  vram=[' ' for i in range(32*BLOCK_H)]
  for i in block:
    line(i[0],i[1])
  for y in range(BLOCK_H):
    for x in range(BLOCK_W):
      o=get_pixel(x,y)
      if o==(0,0,0): vram[y*BLOCK_W+x]='#'
      else: vram[y*BLOCK_W+x]=' '
  for y in range(BLOCK_H):
    for x in range(BLOCK_W):
      if vram[y*BLOCK_W+x] == ' ':
        vram[y*BLOCK_W+x]='t'
        print(x,y)
      else:
        vram[y*BLOCK_W+x]='#'
        break
    for x in range(BLOCK_W-1,-1,-1):
      if vram[y*BLOCK_W+x] == ' ':
        vram[y*BLOCK_W+x]='t'
        print(x,y)
      else:
        vram[y*BLOCK_W+x]='#'
        break
  return vram

def dcube(sx,sy,cube):
  for y in range(BLOCK_H):
    for x in range(BLOCK_W):
      if cube[y*BLOCK_W+x] == ' ': set_pixel(sx+x,sy+y,(255,255,255))
      if cube[y*BLOCK_W+x] == '#': set_pixel(sx+x,sy+y,(0,0,0))

cube=genblock(block)

def render(data,sx,sy,sz):
  global cube
  dx=0
  dy=0
  dz=0
  for z in data:
    dy=0
    z.reverse()
    for y in z:
      dx=0
      for t in y:
        if t == '#':
          dcube((dx*BLOCK_W121+(dz*(-BLOCK_W121)))+sx,(dx+dz)*BLOCK_H131+(dy*(-BLOCK_H131))+sy,cube)
        dx+=1
      dy+=1
    dz+=1

clear_screen()
#dcube(4,8,cube)
render(data,48,16,0)
show_screen()

#eof