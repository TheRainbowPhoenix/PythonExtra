#!/usr/bin/env python3
"""Unit tests for SHM virtual USB ring and loopback bridge."""
import importlib.util
import mmap
import socket
import struct
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("usb_bridge", HERE / "usb_bridge.py")
usb_bridge = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = usb_bridge
spec.loader.exec_module(usb_bridge)


class USBBridgeTests(unittest.TestCase):
    def setUp(self):
        self.mm = mmap.mmap(-1, usb_bridge.MAP_SIZE)
        self.rings = usb_bridge.USBRings(self.mm)
        self.rings.set(0, 1)

    def tearDown(self):
        self.mm.close()

    def test_ring_wrap_and_binary(self):
        self.rings.set(1, usb_bridge.CAP - 2)
        self.rings.set(2, usb_bridge.CAP - 2)
        data = b"\x00\xff\x10\x11\x80"
        self.assertEqual(self.rings.to_calculator(data), len(data))
        self.assertEqual(self.mm[usb_bridge.RX+usb_bridge.CAP-2:usb_bridge.RX+usb_bridge.CAP], data[:2])
        self.assertEqual(self.mm[usb_bridge.RX:usb_bridge.RX+3], data[2:])
        self.rings.set(3, usb_bridge.CAP+5)
        self.rings.set(4, usb_bridge.CAP-2)
        self.mm[usb_bridge.TX+usb_bridge.CAP-2:usb_bridge.TX+usb_bridge.CAP]=b"AB"
        self.mm[usb_bridge.TX:usb_bridge.TX+5]=b"CDEFG"
        self.assertEqual(self.rings.from_calculator(), b"ABCDEFG")
        self.assertEqual(self.rings.used(3,4), 0)

    def test_win32_empty_select_is_never_called(self):
        # On Windows, select([], [], [], timeout) raises WinError 10022.
        from unittest.mock import patch
        real_select = usb_bridge.select.select
        def win_select(readers, writers, errors, timeout=0):
            if not readers and not writers and not errors:
                raise OSError(10022, "Windows invalid argument")
            return real_select(readers, writers, errors, timeout)
        self.rings.set(0, 0)
        stop = threading.Event()
        ready = threading.Event()
        ports = []
        failures = []
        def launch():
            try:
                usb_bridge.bridge(self.mm, port=0, stop=stop,
                    ready=lambda value: (ports.append(value), ready.set()))
            except Exception as error:
                failures.append(error)
        with patch.object(usb_bridge.select, "select", side_effect=win_select):
            thread = threading.Thread(target=launch, daemon=True)
            thread.start()
            self.assertTrue(ready.wait(3))
            try:
                with socket.create_connection(("127.0.0.1", ports[0]), timeout=2):
                    time.sleep(0.04)
                    self.assertFalse(failures)
            finally:
                stop.set()
                thread.join(3)
            self.assertFalse(thread.is_alive())
            self.assertFalse(failures)

    def test_overrun_rejected(self):
        self.rings.set(1,usb_bridge.CAP+1)
        self.assertRaises(ValueError,self.rings.receive_capacity)

    def test_tcp_bidirectional(self):
        ready = threading.Event()
        stop = threading.Event()
        port = []
        def listening(value):
            port.append(value)
            ready.set()
        thread = threading.Thread(target=usb_bridge.bridge,
                                  args=(self.mm,),
                                  kwargs={"port": 0, "ready": listening, "stop": stop},
                                  daemon=True)
        thread.start()
        self.assertTrue(ready.wait(3), "no TCP listener")
        try:
            with socket.create_connection(("127.0.0.1",port[0]),timeout=3) as peer:
                peer.settimeout(3)
                peer.sendall(b"from-host\x00")
                deadline=time.monotonic()+3
                while self.rings.used(1,2)<10 and time.monotonic()<deadline:
                    time.sleep(0.01)
                self.assertEqual(self.rings.used(1,2),10)
                self.assertEqual(self.mm[usb_bridge.RX:usb_bridge.RX+10],b"from-host\x00")
                self.mm[usb_bridge.TX:usb_bridge.TX+8]=b"from-vm!"
                self.rings.set(3,8)
                self.assertEqual(peer.recv(8),b"from-vm!")
        finally:
            stop.set()
            thread.join(4)
            self.assertFalse(thread.is_alive())


if __name__ == "__main__":
    unittest.main()
