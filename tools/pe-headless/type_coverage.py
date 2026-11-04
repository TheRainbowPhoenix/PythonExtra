#!/usr/bin/env python3
"""Runtime gint symbol audit against pythonextra-template/.typings/gint.pyi."""
import argparse
import ast
import json
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--typing", type=Path, required=True)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    tree = ast.parse(args.typing.read_text(encoding="utf-8"))
    required = set()
    for item in tree.body:
        if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)):
            if item.name != "__init__":
                required.add(item.name)
        elif isinstance(item, ast.AnnAssign) and isinstance(item.target, ast.Name):
            if item.target.id != "I":
                required.add(item.target.id)
    code = "import gint;print('|'.join(dir(gint)))"
    p = subprocess.run([str(args.exe.resolve()), "-c", code],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       text=True, timeout=15, check=True)
    actual = set(p.stdout.strip().split("|"))
    missing = sorted(required - actual)
    # A dir(gint) check alone misses properties declared on the image type.
    image_class = next((x for x in tree.body if isinstance(x, ast.ClassDef) and x.name == "image"), None)
    image_attrs = sorted(
        x.target.id for x in image_class.body
        if isinstance(x, ast.AnnAssign) and isinstance(x.target, ast.Name)
    ) if image_class is not None else []
    image_test = (
        "import gint;"
        "im=gint.image(gint.IMAGE_P4_RGB565,16,4,2,2,bytearray(4),bytearray(32));"
        "print('|'.join(x for x in " + repr(image_attrs) + " if not hasattr(im,x)))"
    )
    image_probe = subprocess.run([str(args.exe.resolve()), "-c", image_test],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 text=True, timeout=15)
    image_missing = image_probe.stdout.strip().split("|") if image_probe.stdout.strip() else []
    if image_probe.returncode != 0:
        image_missing = image_attrs
    record = {"expected": len(required), "present": len(required)-len(missing),
              "missing": missing, "image_expected": len(image_attrs),
              "image_missing": image_missing,
              "image_probe_error": image_probe.stderr.strip()[-1000:] if image_probe.returncode else ""}
    print(json.dumps(record, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(record, indent=2)+"\n")
    if missing or image_missing:
        raise SystemExit("gint typing surface or image instance attributes incomplete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
