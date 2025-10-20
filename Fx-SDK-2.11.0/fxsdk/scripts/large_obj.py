#! /usr/bin/env python

import os
import sys
import glob
import subprocess
import elftools.elf.elffile

if len(sys.argv) != 2 or "-h" in sys.argv or "--help" in sys.argv:
    print(f"usage: {sys.argv[0]} <FOLDER>", file=sys.stderr)
    print("Prints sections of *.o/*.obj files in the folder, sorted by size",
        file=sys.stderr)
    sys.exit(1)

files  = glob.glob(sys.argv[1] + "/**/*.o", recursive=True)
files += glob.glob(sys.argv[1] + "/**/*.obj", recursive=True)

prefix = os.path.commonprefix(files)

def get_section_sizes(path, sections):
    fp = open(path, "rb")
    elf = elftools.elf.elffile.ELFFile(fp)

    sizes = dict()
    for s in sections:
        sec = elf.get_section_by_name(s)
        sizes[s] = 0 if sec is None else sec.data_size
    return sizes

SECTIONS = [".text", ".rodata", ".data"]

DB = []
for f in files:
    for s, size in get_section_sizes(f, SECTIONS).items():
        if size > 0:
            DB.append((f, s, size))

DB = sorted(DB, key=lambda triplet: triplet[2])

if not len(DB):
    print("no object files found")
for (f, s, size) in DB:
    print(size, s, f[len(prefix):])

total = sum(size for f, s, size in DB)
print(f"total {total} bytes counted")
