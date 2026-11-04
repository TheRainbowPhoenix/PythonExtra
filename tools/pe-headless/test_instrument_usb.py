#!/usr/bin/env python3
"""End-to-end Windows fxcp_instrument USB -> SHM -> loopback TCP test.

Runs the actual instrumented MicroPython binary with PE_MAP set and a separate
TCP client. No physical USB device or framebuffer serialization is involved.
"""
import argparse
import mmap
import os
import socket
import struct
import subprocess
import tempfile
import threading
import time
from pathlib import Path

from usb_bridge import MAP_SIZE, bridge


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    args = parser.parse_args()
    exe = args.exe.resolve()
    if not exe.is_file():
        parser.error("missing instrumented MicroPython executable")
    with tempfile.TemporaryDirectory(prefix="pe-usb-test-") as directory:
        path = Path(directory) / "screen.mmap"
        with path.open("w+b") as fd:
            fd.truncate(MAP_SIZE)
            with mmap.mmap(fd.fileno(), MAP_SIZE, access=mmap.ACCESS_WRITE) as memory:
                struct.pack_into("<II", memory, 0, 0x50454931, 1)
                ready = threading.Event()
                stop = threading.Event()
                ports = []
                def started(port):
                    ports.append(port)
                    ready.set()
                worker = threading.Thread(target=bridge, args=(memory,),
                    kwargs={"port": 0, "ready": started, "stop": stop}, daemon=True)
                worker.start()
                if not ready.wait(4):
                    raise RuntimeError("USB TCP server did not start")
                script = ("import gint; u=gint.USB();u.open();"
                          "u.write(b'CPChat-to-peer');u.flush();"
                          "print(u.read(16,3000));u.close()")
                env = dict(os.environ, PE_MAP=str(path))
                process = subprocess.Popen([str(exe), "-c", script], cwd=directory,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
                try:
                    with socket.create_connection(("127.0.0.1", ports[0]), timeout=4) as client:
                        client.settimeout(5)
                        outgoing = bytearray()
                        while len(outgoing) < len(b"CPChat-to-peer"):
                            chunk = client.recv(1024)
                            if not chunk:
                                raise RuntimeError("guest closed virtual USB unexpectedly")
                            outgoing.extend(chunk)
                        if bytes(outgoing) != b"CPChat-to-peer":
                            raise AssertionError(f"unexpected guest output {outgoing!r}")
                        client.sendall(b"virtual-usb-reply")
                    out, err = process.communicate(timeout=8)
                    if process.returncode != 0:
                        raise RuntimeError(f"interpreter error {process.returncode}: {err}")
                    if "virtual-usb-reply" not in out:
                        raise AssertionError(f"guest did not receive virtual USB: {out!r} {err!r}")
                    print("PASS: real gint.USB write/read via SHM and TCP:", out.strip())
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.communicate(timeout=4)
                    stop.set()
                    worker.join(timeout=4)


if __name__ == "__main__":
    main()
