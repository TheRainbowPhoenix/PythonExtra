#!/usr/bin/env python3
"""Behavioral compatibility checks for the public gint.pyi API (both host variants).

Checks deliberately avoid checking pictures in the counter-only host: rendering
semantics are separately verified in the full SHM instrumentation test.
"""
import argparse
import subprocess
from pathlib import Path

CASES = {
    "image fields and typed constructor": """
import gint
data=bytearray(4); palette=bytearray(32)
im=gint.image(gint.IMAGE_P4_RGB565,16,4,2,2,data,palette)
assert (im.format,im.width,im.height,im.stride,im.color_count)==(6,4,2,2,16)
assert im.flags>=0 and im.data is data and im.palette is palette
assert gint.image_rgb565(2,2,bytearray(8)).width==2
gint.dimage(0,0,im)
gint.dsubimage(0,0,im,0,0,4,2)
""",
    "text and clipping signatures": """
import gint
a=gint.dwindow_get()
assert len(a)==4
gint.dwindow_set(0,0,40,40)
gint.dtext(2,2,gint.C_BLACK,'hello')
gint.dtext_opt(20,20,gint.C_BLACK,gint.C_NONE,gint.DTEXT_CENTER,gint.DTEXT_MIDDLE,'hello',-1)
assert len(gint.dsize('abc',None))==2
assert len(gint.dnsize('abcdef',3,None))==2
assert len(gint.drsize('abcdef',None,20))==2
gint.drect_border(0,0,15,15,gint.C_NONE,2,gint.C_RED)
gint.dellipse(0,0,16,12,gint.C_NONE,gint.C_RED)
gint.dcircle(12,12,4,gint.C_BLUE,gint.C_NONE)
gint.dpoly([1,1,10,1,7,9],gint.C_GREEN,gint.C_BLACK)
gint.dwindow_set(*a)
""",
    "keyboard and getkey returns": """
import gint
assert gint.keycode_digit(gint.KEY_5)==5
assert gint.keycode_function(gint.KEY_F3)==3
assert isinstance(gint.keydown(gint.KEY_LEFT),bool)
assert isinstance(gint.keypressed(gint.KEY_LEFT),bool)
assert isinstance(gint.keyreleased(gint.KEY_LEFT),bool)
assert len(gint.pollevent())==8
assert len(gint.getkey_opt(gint.GETKEY_NONE,0))==8
""",
}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe",required=True,type=Path)
    parser.add_argument("--timeout",type=int,default=8)
    args=parser.parse_args()
    executable=str(args.exe.resolve())
    errors=[]
    for name,code in CASES.items():
        try:
            proc=subprocess.run([executable,"-c",code],capture_output=True,
                                text=True,timeout=args.timeout)
            if proc.returncode:
                errors.append(name+": "+proc.stderr[-1500:])
                print("FAIL",name)
            else:
                print("PASS",name)
        except subprocess.TimeoutExpired:
            errors.append(name+": timeout")
            print("TIMEOUT",name)
    if errors:
        raise SystemExit("\n".join(errors))

if __name__=="__main__":
    main()
