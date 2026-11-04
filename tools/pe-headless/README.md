# fxcp headless tools on Linux

The 32-bit `ports/unix/build-fxcp_headless/micropython` supports a small
native `gint` tracing stub (input queue and drawing counters). It does **not**
create a framebuffer, a graphical preview or a shared-memory control channel.

Run the bounded sandbox probe against the CI-built executable:

```sh
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython \
    --examples /path/to/MiscPythonExtra --json /tmp/fxcp-results.json
```

To run any program, pass `--program /path/to/file.py`; execution times out
by default after eight seconds. Run untrusted programs inside a real sandbox
with restricted filesystem/network access; a timeout is not a security boundary.

The separate `pe-headless.zip` Windows instrumentation suite provides
`view.py --debug`, `pehost.py`, the inspector, virtual clock, and framebuffer.
It requires the **Windows `fxcp_instrument`** executable, not this Linux
`fxcp_headless` build. Copying that viewer unchanged onto Linux does not
enable UI rendering. Porting the full instrumentation host requires a Linux
IPC/framebuffer/clock backend and a larger `gint` implementation.

Current Linux stub intentionally lacks `dpixel`, `dtext`, `getkey`, and
many normal calculator APIs. The probe tests real MiscPythonExtra compression
libraries and explicitly records graphical games as incompatible rather than
mistaking an expected `AttributeError` for a successful game run.
