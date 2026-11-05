#!/usr/bin/env python3
"""Bridge a PE_HEADLESS Windows instrumentation USB SHM ring to loopback TCP.

One bridge owns the USB ring; independent screen viewers continue reading
screen.mmap without being involved in USB traffic.

  python tools/pe-headless/usb_bridge.py SESSION/screen.mmap --port 8765

Connect a TCP client to 127.0.0.1:8765. The TCP stream corresponds to the
byte stream of gint.USB.write/read; fxlink_header is forwarded verbatim.
"""
import argparse
import mmap
import os
import select
import socket
import struct
import time
from pathlib import Path

MAP_SIZE = 1048576
CAP = 16384
RX = 928000  # TCP -> calculator: USB receive
TX = 944384  # calculator -> TCP: USB transmit
CTRL = 1260  # open, rx_write, rx_read, tx_write, tx_read, flush


class USBRings:
    def __init__(self, memory):
        self.memory = memory

    def get(self, index):
        return struct.unpack_from("<I", self.memory, CTRL + index * 4)[0]

    def set(self, index, value):
        struct.pack_into("<I", self.memory, CTRL + index * 4, value & 0xFFFFFFFF)

    def used(self, write_index, read_index):
        n = (self.get(write_index) - self.get(read_index)) & 0xFFFFFFFF
        if n > CAP:
            raise ValueError("corrupt virtual USB ring counters")
        return n

    def receive_capacity(self):
        return CAP - self.used(1, 2)

    def to_calculator(self, data):
        data = memoryview(data)
        size = min(len(data), self.receive_capacity())
        w = self.get(1)
        first = min(size, CAP - w % CAP)
        self.memory[RX + w % CAP:RX + w % CAP + first] = data[:first]
        if size > first:
            self.memory[RX:RX + size - first] = data[first:size]
        self.set(1, w + size)
        return size

    def from_calculator(self, size=CAP):
        size = min(size, self.used(3, 4))
        r = self.get(4)
        first = min(size, CAP - r % CAP)
        data = bytes(self.memory[TX + r % CAP:TX + r % CAP + first])
        if size > first:
            data += bytes(self.memory[TX:TX + size - first])
        self.set(4, r + size)
        return data


def bridge(memory, host="127.0.0.1", port=8765, ready=None, stop=None):
    if host not in ("127.0.0.1", "::1", "localhost"):
        raise ValueError("only loopback interfaces are permitted")
    rings = USBRings(memory)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", port))
        listener.listen(1)
        listener.settimeout(0.15)
        if ready is not None:
            ready(listener.getsockname()[1])
        while stop is None or not stop.is_set():
            try:
                conn, _ = listener.accept()
            except socket.timeout:
                continue
            with conn:
                conn.setblocking(False)
                outbound = bytearray()
                pending_input = bytearray()
                while stop is None or not stop.is_set():
                    if rings.get(0) == 0:
                        # Wait for the guest to call USB.open().
                        stop.wait(0.01) if stop is not None else time.sleep(0.01)
                        continue
                    if pending_input:
                        n = rings.to_calculator(pending_input)
                        del pending_input[:n]
                    if len(outbound) < CAP:
                        outbound.extend(rings.from_calculator(CAP - len(outbound)))
                    readers = [conn] if len(pending_input) < CAP else []
                    writers = [conn] if outbound else []
                    readable, writable, _ = select.select(readers, writers, [], 0.01)
                    if readable:
                        try:
                            chunk = conn.recv(CAP - len(pending_input))
                        except BlockingIOError:
                            chunk = None
                        if chunk == b"":
                            break
                        if chunk:
                            pending_input.extend(chunk)
                    if writable:
                        try:
                            n = conn.send(outbound)
                        except BlockingIOError:
                            n = 0
                        if n:
                            del outbound[:n]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mapping", type=Path, help="The existing PE_MAP screen.mmap")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    if not 0 < args.port < 65536:
        parser.error("port must be in range 1..65535")
    with args.mapping.open("r+b") as source:
        if os.fstat(source.fileno()).st_size != MAP_SIZE:
            parser.error("expected 1048576-byte instrumentation SHM file")
        with mmap.mmap(source.fileno(), MAP_SIZE, access=mmap.ACCESS_WRITE) as shared:
            if struct.unpack_from("<I", shared, 0)[0] != 0x50454931:
                parser.error("not a PE instrumentation mapping")
            print(f"Virtual gint.USB: tcp://127.0.0.1:{args.port}", flush=True)
            bridge(shared, port=args.port)


if __name__ == "__main__":
    main()
