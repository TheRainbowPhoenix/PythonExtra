# fxcp_headless tests

```sh
python3 tools/pe-headless/usb_bridge.py /path/to/session/screen.mmap --port 8765
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra --json /tmp/probe.json
python3 tools/pe-headless/audit_examples.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra --out /tmp/audit.json
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython --program ../MiscPythonExtra/bounce.py
```

The small Linux/Windows `fxcp_headless` counts calls but cannot display frames.
`view.py --debug` belongs to the separate Windows `fxcp_instrument` host.

The TCP bridge listens on `127.0.0.1` and forwards raw `gint.USB` data, including fxlink headers; it does not emulate an OS USB device or alter SHM screen readers.
