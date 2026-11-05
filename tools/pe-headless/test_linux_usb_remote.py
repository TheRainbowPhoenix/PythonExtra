#!/usr/bin/env python3
"""End-to-end Linux gint.USB vs spu-tools framed remote protocol (no real SPU)."""
import argparse
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import time

PING, STOP, EXEC, STATUS = 0x1001, 0x1002, 0x1003, 0x1004
ERR_UNKNOWN = 0xF0000001

def read_exact(sock, count):
    output = bytearray()
    while len(output) < count:
        chunk = sock.recv(count - len(output))
        if not chunk:
            raise EOFError("USB peer disconnected")
        output.extend(chunk)
    return bytes(output)

def exchange(sock, kind, payload=b""):
    sock.sendall(struct.pack(">II", kind, len(payload)) + payload)
    kind, length = struct.unpack(">II", read_exact(sock, 8))
    return kind, read_exact(sock, length)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--server", type=Path, required=True)
    args = parser.parse_args()
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    env = dict(os.environ, PE_USB_PORT=str(port))
    proc = subprocess.Popen([str(args.exe.resolve()), str(args.server.resolve())],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    client = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                client = socket.create_connection(("127.0.0.1", port), timeout=0.2)
                break
            except OSError:
                if proc.poll() is not None:
                    break
                time.sleep(0.05)
        if client is None:
            raise AssertionError("MicroPython USB server failed to listen")
        with client:
            client.settimeout(8)
            assert exchange(client, PING, b"spu-tools") == (PING, struct.pack(">I", 5) + b"spu-tools")
            print("PASS: spu-tools PING/version")
            assert exchange(client, STATUS, b"hello") == (STATUS, b"")
            print("PASS: STATUS")
            assert exchange(client, EXEC, b"print(6 * 7)") == (EXEC, b"42\n")
            assert exchange(client, EXEC, b"counter = 3") == (EXEC, b"")
            assert exchange(client, EXEC, b"print(counter+1)") == (EXEC, b"4\n")
            print("PASS: remote code execution with persistent context")
            unsupported, body = exchange(client, 0x2003, struct.pack(">II", 0xFE2FFD00, 4))
            assert unsupported == ERR_UNKNOWN and b"SPU hardware not emulated" in body, (unsupported, body)
            print("PASS: SPU memory commands rejected explicitly")
            assert exchange(client, STOP) == (STOP, b"")
            print("PASS: STOP")
        try:
            out, err = proc.communicate(timeout=6)
        except subprocess.TimeoutExpired:
            raise AssertionError("MicroPython server did not stop")
        assert proc.returncode == 0, (proc.returncode, out, err)
        print("PASS: full Linux USB remote transport")
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.communicate(timeout=5)

if __name__ == "__main__":
    main()
