#!/usr/bin/env python3
"""Exercise native opt-in draw recorder, verify duration/geometry/frame grouping."""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ENTRY = struct.Struct("<QIIiiii")
NAMES = {1:"dclear",2:"drect",3:"dpixel",4:"dtext",5:"dline",
         6:"dtext_opt",7:"drect_border",8:"dcircle",9:"dupdate",
         10:"dellipse",11:"dsubimage"}

def load(path):
    raw = Path(path).read_bytes()
    if not raw.startswith(b"PEDRAW1\n"):
        raise AssertionError("wrong draw trace version/header")
    body=raw[8:]
    if len(body) % ENTRY.size:
        raise AssertionError("incomplete trace record")
    return [ENTRY.unpack_from(body,i) for i in range(0,len(body),ENTRY.size)]

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--exe",type=Path,required=True)
    a=p.parse_args()
    script=("import gint;"
            "gint.dclear(gint.C_WHITE);"
            "gint.drect(1,2,3,4,gint.C_BLACK);"
            "gint.dpixel(5,6,gint.C_RED);"
            "gint.dtext(8,9,gint.C_BLACK,'trace');"
            "gint.dline(1,1,6,6,gint.C_BLACK);"
            "gint.dupdate();"
            "gint.drect(4,4,6,6,gint.C_BLUE);"
            "gint.dupdate()")
    with tempfile.TemporaryDirectory() as td:
        trace=Path(td)/"draw.bin"
        env=dict(os.environ,PE_DRAW_TRACE=str(trace))
        completed=subprocess.run([str(a.exe.resolve()),"-c",script],env=env,
                                 capture_output=True,text=True,timeout=15)
        assert completed.returncode == 0, completed.stderr
        records=load(trace)
        methods=[NAMES[r[1]] for r in records]
        assert methods==["dclear","drect","dpixel","dtext","dline","dupdate","drect","dupdate"], methods
        assert records[1][3:7]==(1,2,3,4), records[1]
        assert records[2][3:7]==(5,6,0,0), records[2]
        assert all(r[0]>0 and r[2]>=0 for r in records)
        assert all(a[0]<=b[0] for a,b in zip(records,records[1:]))
        print("PASS: 8 timed calls in 2 captured frames")
        print("Recorded methods:", ", ".join(methods))
if __name__=="__main__":
    main()
