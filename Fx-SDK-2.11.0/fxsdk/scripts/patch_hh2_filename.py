#! /usr/bin/env python

import subprocess
import struct
import sys
import os
import re

eprint = lambda *args, **kwargs: print(*args, **kwargs, file=sys.stderr)
def error(*args, **kwargs):
    eprint("error:", *args, **kwargs)
    sys.exit(1)

if len(sys.argv) != 3:
    eprint(f"usage: {sys.argv[0]} <ELF> <BIN>")
    eprint("Patches filenames into HH2-targeted ELF files for stage-2 load")
    sys.exit(1)

# Check that the filename fits
path = sys.argv[1]
name = os.path.basename(sys.argv[2])
encoded_name = bytes(name, "utf-8")
if len(encoded_name) > 31:
    error(f"output filename {name} is too long (max. 31 bytes)")

# Run a command and return its stdout
def cmd(command):
    rc = subprocess.run(command, stdout=subprocess.PIPE, check=True)
    return rc.stdout.decode("utf-8")

# Get the list of addresses at which a symbol called "name" is defined
def getsym(path, name):
    # All syms as a list of (address, type, name)
    syms = [l.split(" ") for l in cmd(["sh-elf-nm", path]).splitlines()]
    return [int(t[0], 16) for t in syms if t[2] == name]

# Get the section info for the section that contains address "addr"
def getsection(path, addr):
    RE_SECTION = re.compile(
        r'^\s*\[\s*\d+\]\s*([\w._]+)\s+([A-Z]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)')
    lines = cmd(["sh-elf-readelf", "-S", path]).splitlines()

    # Parse sections from the output of readelf
    sections = []
    for l in lines:
        m = RE_SECTION.match(l)
        if m is None:
            continue
        sections.append((m[1], m[2], int(m[3], 16), int(m[4], 16), int(m[5], 16)))

    # Find sections that contain the given address
    matches = []
    for name, kind, start, off, size in sections:
        if kind == "PROGBITS" and start <= addr < start + size:
            matches.append((name, off + (addr - start)))

    return matches

# Get the address of symbol .stage2_filename
s = getsym(path, ".stage2_filename")
if len(s) == 0:
    error(f"no symbol .stage2_filename, is this built for fx-CP?")
if len(s) > 1:
    error(f"multiple ({len(s)}) symbols named .stage2_filename, what?")
filename_address = s[0]

s = getsection(path, filename_address)
if len(s) == 0:
    error(f".stage2_filename is not in any section?")
if len(s) > 1:
    error(f"overlapping sections ({', '.join(e[0] for e in s)}), excuse me?")
if s[0][0] != ".hh2":
    error("how is .stage2_filename not in the .hh2 section?")
name_offset = s[0][1]

# Load file contents
with open(path, "rb+") as fp:
    fp.seek(name_offset)
    name_placeholder = struct.unpack(">8i", fp.read(32))
    if name_placeholder != tuple(range(1, 8+1)):
        error("name placeholder is wrong, check binary format and ELF parsing")

    # Pad to 32 bytes
    encoded_name += bytes(32 - len(encoded_name))
    assert len(encoded_name) == 32

    fp.seek(name_offset)
    fp.write(encoded_name)

sys.exit(0)
