# PacMan par Kevin FEDYNA
# https://nsi.xyz/numapps/pac-man-en-python-numworks/
# converti sur fxCG50 et PythonExtra par SlyVTT

from kandinsky import fill_rect, draw_string
from ion import keydown, KEY_LEFT, KEY_UP, KEY_DOWN, KEY_RIGHT
from math import sqrt
from random import randint
from time import monotonic

try:
    from kandinsky import get_keys
    color = (192, 53, 53)
except:
    color = (255, 183, 52)

terrain = (262143,131841,187245,187245,131073,186285,135969,252783,249903,251823,1152,251823,249903,251823,131841,187245,147465,219051,135969,195453,131073,262143)
bits = 18
width = 320
height = 222
colors = ((0, 0, 0), (32, 48, 248), (248, 224, 8), tuple(color))
ghost_color = ((255, 184, 255), (255, 0,0), (255, 184, 82), (0, 255, 255))
pacgommes = [0,130302,9360,74898,131070,75858,126174,8208,8208,8208,8208,8208,8208,8208,130302,74898,49140,43092,126174,66690,131070,0]
superpacgommes = [0,0,65538,0,0,0,0,0,0,0,0,0,0,0,0,0,65538,0,0,0,0,0,0]
frightened = 0
lives = 2
won = 0
lvl = 0
score = 0
chained = 0


class Entity:
    def __init__(self, x, y, clr, d=0):
        self.x = x
        self.y = y
        self.d = d
        self.nd = d
        self.f = 0
        self.out = 0
        self.color = clr
        fill_rect(int(self.x*10)+136,int(self.y*10)-3,8,8,self.color)
    def espace(self,dx=-1,dy=-1):
        if dx == dy:
            dx, dy = ((-0.1,0),(0,-0.1),(0,0.1),(0.1,0))[self.nd]
        return not terrain[int(self.y + 5.5*dy)]>>(bits-1-int(self.x + 5.5*dx)) & 1 and ((dx != 0 and self.y%1 == 0.5) or (dy != 0 and self.x%1== 0.5))
    def move(self):
        global frightened, ghosts, score, chained, lives, total, won
        dx, dy = ((-0.1,0),(0,-0.1),(0,0.1),(0.1,0))[self.d]
        if self.espace(dx,dy):
            fill_rect(int(self.x*10)+136,int(self.y*10)-3,8,8,colors[0])
            self.x = (round(self.x + dx, 1) - 0.5) % 16.5 + 0.5
            self.y = round(self.y + dy, 1)
            fill_rect(int(self.x*10)+136,int(self.y*10)-3,8,8,self.color)
        if self.color == colors[2]:
            if pacgommes[int(self.y)] >> (bits - 1 - int(self.x)) & 1:
                pacgommes[int(self.y)] -= 1 << (bits - 1 - int(self.x))
                score += 10
            if superpacgommes[int(self.y)] >> (bits - 1 - int(self.x)) & 1:
                superpacgommes[int(self.y)] -= 1 << (bits - 1 - int(self.x))
                score += 50
                chained = 0
                frightened = monotonic()
                for g in ghosts:
                    if g.out:
                        g.color = colors[1]
                        g.d = (3, 2, 1, 0)[g.d]
                        g.f = 1
            for g in range(4):
                if sqrt((self.x-ghosts[g].x)**2+(self.y-ghosts[g].y)**2) < 0.6:
                    if ghosts[g].f:
                        chained += 1
                        total += 1
                        score += (1 << chained)*100
                        ghosts[g].f = 0
                        ghosts[g].color = ghost_color[g]
                        ghosts[g].x = 9
                        ghosts[g].y = 8.5
                        if total == 16:
                            score += 12000
                    else:
                        for gp in range(4):
                            ghosts[gp].f = 0
                            ghosts[gp].color = ghost_color[gp]
                            ghosts[gp].x = 9
                            ghosts[gp].y = 10.5
                            ghosts[gp].out = 0
                        self.x = 9
                        self.y = 16.5
                        self.d, self.nd = 0, 0
                        lives -= 1
                        return render()
            if not won and score > 10000:
                lives += 1
                won = 1
        px, py = int(self.x - 5.5*dx), int(self.y - 5.5*dy)
        if pacgommes[py]>>(bits-1-px) & 1:
            fill_rect(px*10+144,py*10+5,2,2,(250, 207, 173))
        if superpacgommes[py]>>(bits-1-px) & 1:
            fill_rect(px*10+143,py*10+4,4,4,(250, 207, 173))
    def ia(self,x,y):
        if self.f:
            while True:
                d = randint(0,3)
                dx, dy = ((-0.1,0),(0,-0.1),(0,0.1),(0.1,0))[d]
                if d != (3,2,1,0)[self.d] and self.espace(dx,dy):
                    self.d = d
                    break
        else:
            distances = [9999 for _ in range(4)]
            for i in range(4):
                if i != (3,2,1,0)[self.d]:
                    dx, dy = ((-0.1,0),(0,-0.1),(0,0.1),(0.1,0))[i]
                    if self.espace(dx,dy):
                        distances[i] = sqrt((self.y + dy - y)**2 + (self.x + dx - x)**2)
            self.d = distances.index(min(distances))

def prebuild():
    fill_rect(0,0,width,height,colors[0])
    fill_rect(138, 0, 2, height, colors[3])
    draw_string("PAC-MAN", 35, 10, colors[3], colors[0])
    draw_string("nsi.xyz/pacman", 0, 204,colors[0], colors[3])
    draw_string("Score :", 35, 40, (255,)*3, colors[0])
    draw_string("Niveau :", 30, 90, (255,)*3, colors[0])

def render():
    global terrain, pacgommes, superpacgommes, lives, arrivee
    if lives == -1:
        return 42
    draw_string(str(lvl),70-5*len(str(lvl)),110,(255,)*3,colors[0])
    fill_rect(0,150,138,20,colors[0])
    for i in range(lives):
        fill_rect(60-(lives-1)*20+i*40,150,20,20,colors[2])
    for l in range(len(terrain)):
        for c in range(bits):
            fill_rect(c*10+140,l*10+1,10,10,colors[0])
            if pacgommes[l]>>(bits-1-c) & 1:
                fill_rect(c*10+144,l*10+5,2,2,(250, 207, 173))
            if superpacgommes[l]>>(bits-1-c) & 1:
                fill_rect(c*10+143,l*10+4,4,4,(250, 207, 173))
            if terrain[l]>>(bits-1-c) & 1:
                for d in ((1,0),(0,1),(-1,0),(0,-1)):
                    if 0 <= l + d[0] <= len(terrain) - 1 and 0 <= c + d[1] <= bits - 1 and not terrain[l + d[0]]>>(bits-1-(c+d[1])) & 1:
                        fill_rect(c*10+140+9*(d[1]==1),l*10+1+9*(d[0]==1),1+9*(d[1]==0),1+9*(d[0]==0),colors[1])
    arrivee = monotonic()

def engine():
    global frightened, ghosts, pacgommes, superpacgommes, lvl, arrivee, total
    while True:
        pacgommes = [0,130302,9360,74898,131070,75858,126174,8208,8208,8208,8208,8208,8208,8208,130302,74898,49140,43092,126174,66690,131070,0]
        superpacgommes = [0,0,65538,0,0,0,0,0,0,0,0,0,0,0,0,0,65538,0,0,0,0,0,0]
        lvl += 1
        total = 0
        render()
        pacman = Entity(9, 16.5, colors[2])
        ghosts = [Entity(9, 10.5, ghost_color[i]) for i in range(4)]
        while sum(pacgommes) + sum(superpacgommes):
            depart = monotonic()
            lk=[KEY_LEFT, KEY_UP, KEY_DOWN, KEY_RIGHT]
            for i in range(4):
                if keydown(lk[i]):
                    if i == (3,2,1,0)[pacman.d]:
                        pacman.d = i
                    pacman.nd = i
            while monotonic() - depart < 0.01:
                if pacman.espace():
                    pacman.d = pacman.nd
                if pacman.move() == 42:
                    draw_string("GAME OVER",185,100,colors[3],colors[0])
                    return 69

            draw_string(str(score),70-5*len(str(score)),60,(255,)*3,colors[0])

            """ Fantomes """

            if frightened:
                if monotonic() - frightened > 6.5:
                    for g in ghosts:
                        if g.f:
                            g.color = (255,)*3
                if monotonic() - frightened > 8.5:
                    frightened = 0
                    for g in range(4):
                        ghosts[g].color = ghost_color[g]
                        ghosts[g].f = 0

            if arrivee:
                if monotonic() - arrivee > 0 and not ghosts[1].out:
                    ghosts[1].out = 1
                    ghosts[1].y = 8.5
                if monotonic() - arrivee > 2.5 and not ghosts[0].out:
                    ghosts[0].out = 1
                    ghosts[0].y = 8.5
                if monotonic() - arrivee > 5 and not ghosts[3].out:
                    ghosts[3].out = 1
                    ghosts[3].y = 8.5
                if monotonic() - arrivee > 7.5 and not ghosts[2].out:
                    ghosts[2].out = 1
                    ghosts[2].y = 8.5
                    fill_rect(220,101,20,10,colors[0])
                    arrivee = 0

            pdx, pdy = ((-0.1,0),(0,-0.1),(0,0.1),(0.1,0))[pacman.d]

            # Pinky
            ghosts[0].ia(pacman.x + 20 * pdx, pacman.y + 20 * pdy)
            ghosts[0].move()

            # Inky
            ghosts[3].ia(max(min(ghosts[1].x + 2*(pacman.x + 20 * pdx - ghosts[1].x), 16.5), 1.5), max(min(ghosts[1].y +2*(pacman.y + 20 * pdy - ghosts[1].y), 21.5), 1.5))
            ghosts[3].move()

            # Blinky
            ghosts[1].ia(pacman.x, pacman.y)
            ghosts[1].move()

            # Clyde
            if sqrt((ghosts[2].x - pacman.x)**2 + (ghosts[2].y - pacman.y)**2) > 4:
                ghosts[2].ia(pacman.x, pacman.y)
            else:
                ghosts[2].ia(1.5, 20.5)
            ghosts[2].move()


fill_rect(0,0,319,219,"black")
prebuild()
engine()
