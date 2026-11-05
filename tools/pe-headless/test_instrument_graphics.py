#!/usr/bin/env python3
"""Full Windows SHM instrumentation per-draw recording (opt in, no UI hooks)."""
import argparse
import mmap
import os
from pathlib import Path
import struct
import subprocess
import tempfile

SIZE=1048576
MAGIC=0x50454931
FLAG=0x100
CTRL=1024
EVENT_BASE=400000
EVENT_SIZE=256
METHOD={41:"dclear",42:"drect",43:"dpixel",44:"dtext",
        45:"dline",49:"dupdate"}

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--exe",type=Path,required=True)
    a=p.parse_args()
    with tempfile.TemporaryDirectory() as temp:
        path=Path(temp)/"screen.mmap"
        with path.open("w+b") as f:
            f.truncate(SIZE)
            with mmap.mmap(f.fileno(),SIZE,access=mmap.ACCESS_WRITE) as map:
                struct.pack_into("<II",map,0,MAGIC,1)
                struct.pack_into("<I",map,17*4,FLAG)
                script=("import gint;"
                        "gint.dclear(gint.C_WHITE);"
                        "gint.drect(0,0,3,3,gint.C_BLUE);"
                        "gint.dpixel(4,4,gint.C_RED);"
                        "gint.dtext(6,7,gint.C_BLACK,'abc');"
                        "gint.dline(0,1,3,4,gint.C_RED);"
                        "gint.dupdate()")
                env=dict(os.environ,PE_MAP=str(path))
                proc=subprocess.run([str(a.exe.resolve()),"-c",script],
                    env=env,capture_output=True,text=True,timeout=20)
                assert proc.returncode==0,proc.stderr
                w,r,drops=struct.unpack_from("<III",map,CTRL)
                names=[]
                for i in range(r,w):
                    offset=EVENT_BASE+(i%1024)*EVENT_SIZE
                    timestamp,kind,micros,extra=struct.unpack_from("<QIII",map,offset)
                    if kind in METHOD:
                        label=bytes(map[offset+20:offset+256]).split(b"\x00",1)[0]
                        label=label.decode("utf-8")
                        assert label.split("\n",1)[0]==METHOD[kind],label
                        # Every recorded gint method includes its Python
                        # call site in the *same* native SPSC trace event.
                        frames=label.split("\n")[1:]
                        assert frames and "|" in frames[0],label
                        assert timestamp>0 and micros<10000000
                        names.append(METHOD[kind])
                expected=["dclear","drect","dpixel","dtext","dline","dupdate"]
                assert names==expected,(names,w,r,drops)
                assert not drops
                # Other independent screen readers get the normal RGB565 data.
                with path.open("rb") as fd:
                    with mmap.mmap(fd.fileno(),SIZE,access=mmap.ACCESS_READ) as screen:
                        color=struct.unpack_from("<H",screen,4096)[0]
                        assert color==gint_blue(),hex(color)
                print("PASS: 6 timed SHM Graphics calls, normal RGB565 VRAM and no drops")

def gint_blue():
    return 31

if __name__=="__main__":
    main()
