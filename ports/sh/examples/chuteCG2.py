from gint import *

#---

from math import sin, cos, pi
from random import randint

pattern = 2
def world(z0, dir, r=None):
    r = randint(1, 3)
    r = 3
    if r == 1:
        return world_red(z0, dir)
    elif r == 2:
        return world_green(z0, dir)
    elif r == 3:
        return world_blue(z0, dir)

def world_red(z0, dir):
    length = randint(2, 4)
    platforms = [None] * (length * 8)
    for i in range(length):
        for direction in range(4):
            platforms[8*i+direction] = [0, direction, z0+8*i, z0+8*i+8, 2]
        for j in range(4):
            obstacle = randint(0,3)
            platforms[8*i+4+j] = [1, obstacle, z0+8*i+2*j+1, z0+8*i+2*j+1, 2]
    return platforms, z0+length*8, (obstacle+1)&3

def world_green(z0, dir):
    length = randint(8, 16)
    platforms = [None] * length
    for i in range(length):
        platforms[i] = [0, dir, z0+4+2*i, z0+8+2*i, 3]
        dir = (dir - randint(1,2)) & 3
    platforms[0][2] = z0
    return platforms, z0+(length-1)*2+8, dir

def world_blue(z0, dir):
    platforms = []
    for i in range(16):
        platforms.append([0, dir, z0+2*i, z0+4+2*i, 4])
        if i <= 14 and randint(0,2) == 0:
            bait = (dir + randint(1,3)) & 3
            platforms.append([0, bait, z0+2*i, z0+6+2*i, 4])
            platforms.append([1, bait, z0+3+2*i, z0+3+2*i, 4])
        dir = (dir - randint(1,2)) & 3
    return platforms, z0+34, dir

def safe_position(platforms, z, direction):
    supported = False
    # Messed up direction encoding
    if direction & 1:
        direction = direction ^ 2
    for (kind, dir, z1, z2, color) in platforms:
        if dir != direction:
            continue
        if kind == 1 and z > z1-1 and z < z2:
            return False
        if kind == 0 and z > z1-0.5 and z < z2:
            supported = True
    return supported

def gen_vertices(platforms, index):
    vertices = []
    for (kind, direction, z1, z2, color) in platforms:
        if kind == 0:
            vertices.append([-4,-3,z1,index])
            vertices.append([-4, 3,z1,index])
            vertices.append([-4, 3,z2,index])
            vertices.append([-4,-3,z2,index])
        elif kind == 1:
            vertices.append([-3,-.5,z1,index])
            vertices.append([-2, .5,z1,index])
            vertices.append([-3, .5,z1,index])
            vertices.append([-2,-.5,z1,index])
        index += 1

    return vertices

def platform_rotation(vertices, platforms, world_offset):
    for i in range(len(vertices)):
        x, y, z, p = vertices[i]
        dir = platforms[p+world_offset][1]
        c = (dir == 1) - (dir == 3)
        s = (dir == 2) - (dir == 0)
        vertices[i] = [c*x+s*y,s*x+c*y, z, p]

def transform3d(vertices, theta, camera_z, rotate, flip):
    theta -= rotate * pi / 14
    f = 1 - flip / 3.5
    s, c = sin(theta), cos(theta)

    for i in range(len(vertices)):
        x, y, z, p = vertices[i]
        vertices[i] = [(c*x-s*y)*f,(s*x+c*y)*f,(z-camera_z)*2,p]

def draw_line(p1, p2, color):
    if p1[2] <= 1:
        p1, p2 = p2, p1
    if p1[2] <= 1:
        return
    if p2[2] <= 1:
        t = (p1[2] - 1) / (p1[2] - p2[2])
        p2[0] = p1[0] + t * (p2[0] - p1[0])
        p2[1] = p1[1] + t * (p2[1] - p1[1])
        p2[2] = 1

    x = 196 + int(75 * p1[0] / p1[2])
    y = 112 + int(75 * p1[1] / p1[2])

    x2 = 196 + int(75 * p2[0] / p2[2])
    y2 = 112 + int(75 * p2[1] / p2[2])

    dline(x, y, x2, y2, color)

def draw_scene(vertices, platforms, world_offset, colors, z, score,
    clear=False):
    if clear:
        drect(163, 153, 220, 165, colors[0])

    maxdepth = min(len(vertices), 48)
    i = 0
    while i < len(vertices):
        if vertices[i][2] > 16:
            break
        p = platforms[vertices[i][3]+world_offset]
        c = colors[0 if clear else p[4]]

        if p[0] == 0:
            draw_line(vertices[i], vertices[i+1], c)
            draw_line(vertices[i+1], vertices[i+2], c)
            draw_line(vertices[i+2], vertices[i+3], c)
            draw_line(vertices[i+3], vertices[i], c)
            i += 4
        elif p[0] == 1:
            draw_line(vertices[i], vertices[i+1], c)
            draw_line(vertices[i+2], vertices[i+3], c)
            i += 4

    if not clear:
        dtext_opt(DWIDTH//2, DHEIGHT-15, colors[1], colors[0], DTEXT_CENTER,
            DTEXT_BOTTOM, " " + str(score) + " ", -1)

    dtext_opt(DWIDTH//2, 15, colors[1], colors[0], DTEXT_CENTER, DTEXT_BOTTOM,
        "Chute tridimensionnelle", -1)

colors_light = [
    0xffff, # White
    0x0000, # Black
    0xc000, # Dark red
    0x0480, # Dark green
    0x0018, # Dark blue
]
colors_dark = [
    0x0000, # Black
    0xffff, # White
    0xfd14, # Light red
    0x87f0, # Light green
    0xa51f, # Light blue
]
colors = colors_light

dclear(colors[0])

z = 0
score = 0
vertices = []
direction = 0
rotate = 0
flip = 0

world_platforms, world_end, world_dir = world_green(z, direction)
world_offset = 0

while True:
    if colors == colors_light and score >= 333:
        dclear(colors_dark[0])
        colors = colors_dark
    else:
        draw_scene(vertices, world_platforms, world_offset, colors, z,
            score, True)

    vertices = gen_vertices(world_platforms, -world_offset)
    platform_rotation(vertices, world_platforms, world_offset)
    transform3d(vertices, direction * pi / 2, z, rotate, flip)
    draw_scene(vertices, world_platforms, world_offset, colors, z, score)
    dupdate()

    if z >= world_platforms[0][3]:
        world_platforms.pop(0)
        world_offset -= 1
    if len(world_platforms) < 16:
        p, world_end, world_dir = world(world_end, world_dir)
        world_platforms += p

    z += .25

    if rotate or flip:
        pass
    elif safe_position(world_platforms, z, direction):
        score += 1
    else:
        score += 1#max(score - 8, 0)

    rotate -= (rotate > 0)
    flip -= (flip > 0)

    while 1:
        ev = pollevent()
        if ev.type == KEYEV_NONE:
            break

        if ev.type == KEYEV_DOWN and ev.key in [KEY_SHIFT, KEY_EXE, KEY_RIGHT]:
            if rotate >= 5:
                direction = (direction + 1) & 3
                flip = rotate
                rotate = 0
            elif rotate == 0:
                direction = (direction + 1) & 3
                rotate = 7
