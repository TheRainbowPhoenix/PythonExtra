from gint import *

data1 = [
  ["    ", "    ", "    ", "    ", "####"],
  ["### ", "    ", "    ", "    ", "####"],
  ["### ", " #  ", " #  ", " #  ", "####"],
  ["### ", "    ", "    ", "    ", "####"]]
data2 = [
  ["    ", " ###", " ###", "    ", "####"],
  [" ###", " ###", " ###", " # #", "####"],
  [" ###", " ###", " ###", " # #", "####"],
  ["    ", " ###", "    ", " # #", "####"]]

def line(pos1,pos2):
  x1=int(pos1[0])
  y1=int(pos1[1])
  x2=int(pos2[0])
  y2=int(pos2[1])
  dline(x1, y1, x2, y2, C_BLACK)

BLOCK_H=24
BLOCK_W=24

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
      o=dgetpixel(x,y)
      if o==C_BLACK: vram[y*BLOCK_W+x]='#'
      else: vram[y*BLOCK_W+x]=' '
  for y in range(BLOCK_H):
    for x in range(BLOCK_W):
      if vram[y*BLOCK_W+x] == ' ':
        vram[y*BLOCK_W+x]='t'
        #print(x,y)
      else:
        vram[y*BLOCK_W+x]='#'
        break
    for x in range(BLOCK_W-1,-1,-1):
      if vram[y*BLOCK_W+x] == ' ':
        vram[y*BLOCK_W+x]='t'
        #print(x,y)
      else:
        vram[y*BLOCK_W+x]='#'
        break
  return vram

def dcube(sx,sy,cube):
  for y in range(BLOCK_H):
    for x in range(BLOCK_W):
      if cube[y*BLOCK_W+x] == ' ': dpixel(sx+x,sy+y,C_WHITE)
      if cube[y*BLOCK_W+x] == '#': dpixel(sx+x,sy+y,C_BLACK)

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

dclear(C_WHITE)
render(data1,48,32,0)
dupdate()
getkey()

dclear(C_WHITE)
render(data2,48,32,0)
dupdate()
getkey()
