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
    record = {"expected": len(required), "present": len(required)-len(missing),
              "missing": missing}
    print(json.dumps(record, indent=2))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(record, indent=2)+"\n")
    if missing:
        raise SystemExit("gint typing coverage incomplete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
