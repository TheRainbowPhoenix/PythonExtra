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
                script = ("import gint; gint.drect(0,0,3,3,0x1234); gint.dupdate(); "
                          "u=gint.USB();u.open();"
                          "u.fxlink_header('CPChat','chat',12);"
                          "u.write(b'chat-message');"
                          "u.fxlink_header('CPChat','bulk',24576);"
                          "u.write(b'Z'*24576);u.flush();"
                          "print(u.read(17,3000));u.close()")
                env = dict(os.environ, PE_MAP=str(path))
                process = subprocess.Popen([str(exe), "-c", script], cwd=directory,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
                try:
                    with socket.create_connection(("127.0.0.1", ports[0]), timeout=4) as client:
                        client.settimeout(5)
                        outgoing = bytearray()
                        header_size = struct.calcsize("<III16s16s")
                        total = header_size * 2 + len(b"chat-message") + 24576
                        while len(outgoing) < total:
                            chunk = client.recv(1024)
                            if not chunk:
                                raise RuntimeError("guest closed virtual USB unexpectedly")
                            outgoing.extend(chunk)
                        header = struct.unpack_from("<III16s16s", outgoing)
                        if header[:3] != (0x100, 12, 2048):
                            raise AssertionError(f"invalid fxlink header {header!r}")
                        if header[3].split(b"\\0", 1)[0] != b"CPChat" or header[4].split(b"\\0", 1)[0] != b"chat":
                            raise AssertionError(f"incorrect fxlink application/type {header!r}")
                        offset = header_size + len(b"chat-message")
                        if bytes(outgoing[header_size:offset]) != b"chat-message":
                            raise AssertionError("unexpected first fxlink payload")
                        bulk = struct.unpack_from("<III16s16s", outgoing, offset)
                        if bulk[:3] != (0x100, 24576, 2048):
                            raise AssertionError(f"invalid second fxlink header {bulk!r}")
                        if bulk[4].split(b"\\0", 1)[0] != b"bulk":
                            raise AssertionError(f"unexpected fxlink bulk type {bulk!r}")
                        if bytes(outgoing[offset+header_size:]) != b"Z"*24576:
                            raise AssertionError("corrupted streamed USB bulk payload")
                        client.sendall(b"virtual-usb-reply")
                    out, err = process.communicate(timeout=8)
                    if process.returncode != 0:
                        raise RuntimeError(f"interpreter error {process.returncode}: {err}")
                    if "virtual-usb-reply" not in out:
                        raise AssertionError(f"guest did not receive virtual USB: {out!r} {err!r}")
                    # A second independent screen reader must still see VRAM
                    # while the USB bridge owns only the dedicated USB rings.
                    with path.open("rb") as screen_fd:
                        with mmap.mmap(screen_fd.fileno(), MAP_SIZE, access=mmap.ACCESS_READ) as screen:
                            if struct.unpack_from("<H", screen, 4096)[0] != 0x1234:
                                raise AssertionError("screen reader lost the published frame")
                            if struct.unpack_from("<H", memory, 4096)[0] != 0x1234:
                                raise AssertionError("USB bridge damaged the shared framebuffer")
                    print("PASS: fxlink USB over TCP and independent SHM screen reader:", out.strip())
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.communicate(timeout=4)
                    stop.set()
                    worker.join(timeout=4)


if __name__ == "__main__":
    main()
