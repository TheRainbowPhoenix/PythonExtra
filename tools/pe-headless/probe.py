#!/usr/bin/env python3
"""Bounded, standard-library Linux fxcp_headless smoke/compatibility harness.

The Unix fxcp_headless is a counter-only gint stub. It does not publish a
framebuffer and does not speak the Windows pehost shared-memory protocol.
"""
import argparse
import json
import subprocess
import sys
from pathlib import Path


def execute(exe, code, timeout):
    try:
        proc = subprocess.run([str(exe), "-c", code], capture_output=True, text=True,
                              timeout=timeout, check=False)
        return {"returncode": proc.returncode, "stdout": proc.stdout[-3000:],
                "stderr": proc.stderr[-3000:], "timeout": False}
    except subprocess.TimeoutExpired as exc:
        return {"returncode": None, "stdout": "", "stderr": str(exc), "timeout": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("ports/unix/build-fxcp_headless/micropython"))
    parser.add_argument("--examples", type=Path, help="Checkout of MiscPythonExtra")
    parser.add_argument("--program", type=Path, help="Run a single script with a time limit")
    parser.add_argument("--timeout", type=float, default=8)
    parser.add_argument("--json", type=Path, help="Write detailed JSON results")
    args = parser.parse_args()
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error("missing Linux MicroPython executable: " + str(exe))
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    cases = [
        ("gint tracing", "import gint,struct; assert struct.calcsize('P')==4; gint.trace_reset(); gint.drect(0,0,9,13,0); gint.dupdate(); assert gint.trace_stats()==(1,1,0,140); print('OK tracing')", "pass")
    ]
    if args.examples:
        root = args.examples.resolve()
        if not root.is_dir():
            parser.error("missing examples directory: " + str(root))
        root_literal = repr(str(root))
        cases += [
            ("MiscPythonExtra/lzw.py", "import sys; sys.path.insert(0,"+root_literal+"); import lzw; b=b'TOBEORNOTTOBEORTOBEORNOT'; assert lzw.decompress(lzw.compress(b))==b; print('OK LZW')", "pass"),
            ("MiscPythonExtra/compression.py", "import sys; sys.path.insert(0,"+root_literal+"); import compression; b=b'ABCD'*50; assert compression.rle_decompress(compression.rle_compress(b))==b; assert compression.lzw_decompress(compression.lzw_compress(b))==b; print('OK compression')", "pass"),
        ]
        # Exercise actual game source with a bounded top-level loop.
        # No changes are made to the upstream samples in MiscPythonExtra.
        for app, loop_pattern in (("bounce.py", "while True:"),
                                  ("asteroids.py", "    while True:")):
            file = root / app
            if not file.is_file():
                parser.error("missing example: " + str(file))
            code = ("import gint; src=open("+repr(str(file))+",'r').read(); "
                    "assert "+repr(loop_pattern)+" in src; "
                    "src=src.replace("+repr(loop_pattern)+", "+
                    repr(loop_pattern.replace("while True:", "for __headless_frame in range(3):"))+", 1); "
                    "exec(compile(src,"+repr(str(file))+",'exec'),{'__name__':'__main__'}); "
                    "assert gint.trace_stats()[0] >= 3; print('OK 3 frames')")
            cases.append(("MiscPythonExtra/"+app+" (3 frames)", code, "pass"))

        file = root / "render_mandelbrot.py"
        if not file.is_file():
            parser.error("missing example: " + str(file))
        # Limit the scanlines to keep a CI probe short; execute real pixel math.
        code = ("import gint; src=open("+repr(str(file))+",'r').read(); "
                "assert 'range(264)' in src; src=src.replace('range(264)','range(8)',1); "
                "exec(compile(src,"+repr(str(file))+",'exec'),{'__name__':'__main__'}); "
                "assert gint.trace_stats()[0] >= 2; print('OK Mandelbrot scanlines')")
        cases.append(("MiscPythonExtra/render_mandelbrot.py (8 scanlines)", code, "pass"))
    if args.program:
        p = args.program.resolve()
        cases.append((str(p), "exec(compile(open("+repr(str(p))+",'rb').read(),"+repr(str(p))+",'exec'),{'__name__':'__main__'})", "manual"))

    results = []
    bad = False
    for name, code, expected in cases:
        run = execute(exe, code, args.timeout)
        combined = run["stdout"] + run["stderr"]
        if expected == "pass":
            ok = run["returncode"] == 0 and not run["timeout"]
            status = "PASS" if ok else "FAIL"
            bad |= not ok
        else:
            status = "PASS" if run["returncode"] == 0 else "TIMEOUT" if run["timeout"] else "ERROR"
            bad |= status != "PASS"
        results.append({"name": name, "expected": expected, "status": status, **run})
        print(f"{status}: {name}")
        if status in ("FAIL", "UNEXPECTED", "ERROR", "TIMEOUT"):
            print(combined[-1000:], file=sys.stderr)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(results, indent=2) + "\n")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
