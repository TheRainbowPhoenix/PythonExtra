"""Minimal spu-tools remote packet server for fxcp_headless.

Run with Linux MicroPython, NOT CPython:
  micropython device/spu_remote_server.py
PE_USB_PORT controls the 127.0.0.1 listener.
Only PING, STATUS, EXEC and STOP are supported. SPU memory is deliberately
not mapped: no unsafe raw register accesses or DSP emulation.
"""
import gint
import struct

PING = 0x1001
STOP = 0x1002
EXEC = 0x1003
STATUS = 0x1004
ERR_OTHER = 0xf0000000
ERR_UNKNOWN = 0xf0000001
MAX_PACKET = 1024 * 1024

def read_exact(usb, count):
    result = bytearray()
    while len(result) < count:
        chunk = usb.read(min(16384, count - len(result)))
        if not chunk:
            raise EOFError("remote disconnected")
        result.extend(chunk)
    return bytes(result)

def send(usb, kind, data):
    usb.write(struct.pack(">II", kind, len(data)))
    # Avoid giant transient C allocations on small MicroPython builds.
    for index in range(0, len(data), 8192):
        usb.write(memoryview(data)[index:index + 8192])
    usb.flush()

def exec_script(source, context):
    output = []
    def capture(*args, **kwargs):
        sep = kwargs.get("sep", " ")
        end = kwargs.get("end", "\n")
        output.append(sep.join(str(a) for a in args) + end)
    context["print"] = capture
    exec(source, context)
    return "".join(output).encode("utf-8")

def serve_one(usb, context):
    header = read_exact(usb, 8)
    kind, length = struct.unpack(">II", header)
    if length > MAX_PACKET:
        # close session to avoid parsing a desynchronized packet stream
        raise ValueError("packet too large")
    payload = read_exact(usb, length)
    if kind == PING:
        send(usb, PING, struct.pack(">I", 5) + payload)
    elif kind == STATUS:
        context["_status"] = payload.decode("utf-8")
        send(usb, STATUS, b"")
    elif kind == EXEC:
        try:
            result = exec_script(payload.decode("utf-8"), context)
        except Exception as exc:
            send(usb, ERR_OTHER, ("%s: %s" % (type(exc).__name__, exc)).encode("utf-8"))
        else:
            send(usb, EXEC, result)
    elif kind == STOP:
        send(usb, STOP, b"")
        return False
    else:
        send(usb, ERR_UNKNOWN, ("Packet 0x%x unsupported (SPU hardware not emulated)" % kind).encode("utf-8"))
    return True

def main():
    context = {"__name__": "__remote__", "gint": gint}
    with gint.USB() as usb:
        while serve_one(usb, context):
            pass

if __name__ == "__main__":
    main()
