#!/usr/bin/env python3
"""Audit fxcp_headless against MiscPythonExtra without modifying examples.

Static audit: compile every .py, compare gint calls/constants with native module.
Dynamic audit: run a bounded set of real entry points in isolated working dirs.
Timeouts are *not* safety isolation; run only trusted example repositories.
"""
import argparse
import ast
import json
import re
import subprocess
import tempfile
from pathlib import Path

API = {
    "getkey", "getkey_opt", "keydown", "keydown_any", "keydown_all",
    "keypressed", "keyreleased", "clearevents", "cleareventflips", "pollevent",
    "keycode_digit", "keycode_function", "C_RGB",
    "dclear", "dupdate", "dpixel", "dgetpixel", "drect", "drect_border",
    "dline", "dhline", "dvline", "dcircle", "dellipse", "dpoly",
    "dwindow_get", "dwindow_set", "dimage", "dsubimage", "dfont",
    "dsize", "dnsize", "drsize", "dtext", "dtext_opt",
    "image", "image_rgb565", "image_rgb565a", "image_p4_rgb565",
    "image_p4_rgb565a", "image_p8_rgb565", "image_p8_rgb565a", "USB",
}
CONSTANT = re.compile(r"^(?:KEY(?:EV)?_[A-Z0-9_]+|C_[A-Z0-9_]+|DTEXT_[A-Z0-9_]+|IMAGE_[A-Z0-9_]+|D(?:WIDTH|HEIGHT)|font(?:8x9|_9))$")
RUN = [
    "bounce.py", "asteroids.py", "render_mandelbrot.py", "render_raytracer.py",
    "neuro.py", "physchem.py", "cgui_demo.py", "cinput_demo.py",
    "cpaint_demo.py", "cptime_view.py", "md_viewer.py", "mobi_view.py",
    "qr_gen.py", "synth3xosc.py", "tinydaw4.py", "svf_editor.py",
    "cdateinput.py", "ced_new.py", "HarmoniExtra.py", "MathGPT.py",
]

def run(exe, code, *, timeout=3, cwd=None):
    try:
        p = subprocess.run([str(exe), "-c", code], cwd=cwd,
                           capture_output=True, text=True, timeout=timeout)
        return {"exit_code": p.returncode, "stdout": p.stdout[-1500:],
                "stderr": p.stderr[-1500:], "timeout": False}
    except subprocess.TimeoutExpired:
        return {"exit_code": None, "stdout": "", "stderr": "",
                "timeout": True}

def inspect_file(file, exposed):
    source = file.read_text(encoding="utf-8-sig")
    try:
        tree = ast.parse(source, filename=str(file))
    except SyntaxError as exc:
        return {"syntax_error": str(exc), "missing_gint": []}
    is_gint = False
    imports_star = False
    attributes = set()
    explicit = set()
    written = set()
    loaded = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            is_gint |= any(x.name == "gint" for x in node.names)
        elif isinstance(node, ast.ImportFrom) and node.module == "gint":
            is_gint = True
            imports_star |= any(x.name == "*" for x in node.names)
            explicit.update(x.name for x in node.names if x.name != "*")
        elif isinstance(node, ast.Attribute) and isinstance(node.value, ast.Name) and node.value.id == "gint":
            attributes.add(node.attr)
        elif isinstance(node, ast.Name):
            if isinstance(node.ctx, ast.Store):
                written.add(node.id)
            elif isinstance(node.ctx, ast.Load):
                loaded.add(node.id)
    if imports_star:
        explicit.update(n for n in loaded - written if n in API or CONSTANT.match(n))
    needed = attributes | explicit
    return {"gint_import": is_gint, "gint_used": sorted(needed),
            "missing_gint": sorted(needed - exposed), "syntax_error": None}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("--examples", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--timeout", type=float, default=1.5)
    args = p.parse_args()
    exe, root = args.exe.resolve(), args.examples.resolve()
    response = run(exe, "import gint; print('|'.join(dir(gint)))")
    if response["exit_code"] != 0:
        raise SystemExit("cannot inspect native gint: "+str(response))
    exposed = set(response["stdout"].strip().split("|"))
    records = {}
    for file in sorted(root.rglob("*.py")):
        if any(x in file.parts for x in (".git", ".typings", "tools", ".devcontainer")):
            continue
        records[file.relative_to(root).as_posix()] = inspect_file(file, exposed)

    executions = {}
    with tempfile.TemporaryDirectory(prefix="fxcp-audit-") as temporary:
        for name in RUN:
            file = root / name
            if not file.is_file():
                continue
            # Compile/run actual program in a fresh child; keep the source tree
            # read-only by putting writes in a scratch working directory.
            code = ("import sys;sys.path.insert(0,"+repr(str(root))+");"
                    "exec(compile(open("+repr(str(file))+",'rb').read(),"+
                    repr(str(file))+",'exec'),{'__name__':'__main__'})")
            result = run(exe, code, timeout=args.timeout, cwd=temporary)
            if result["timeout"]:
                outcome = "TIMEOUT / interactive"
            elif result["exit_code"] == 0:
                outcome = "EXIT 0"
            else:
                outcome = "ERROR"
            executions[name] = {"outcome": outcome, **result}

    report = {"binary": str(exe), "examples": str(root),
              "exposed_gint": sorted(exposed), "files": records,
              "executions": executions}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    problematic = [(n, x["missing_gint"]) for n,x in records.items() if x["missing_gint"]]
    print("Audited",len(records),"Python files;", len(problematic),"have missing gint symbols")
    for name, missing in problematic:
        print("MISSING",name,":",", ".join(missing))
    for name, result in executions.items():
        print("RUN",name,":",result["outcome"],
              result["stderr"].splitlines()[-1:] if result["stderr"] else "")
    # This is a characterization audit, not a green light for compatibility.
    # The strict, bounded probes in probe.py provide pass/fail regressions.
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
