# fxcp_headless

Opt-in **32-bit Windows** MicroPython variant with a native, deliberately partial
`gint` stub. The normal Windows build and the fxcp firmware are unchanged.

Build from ports/windows with a MinGW compiler capable of 32-bit output:

```
make -j4 VARIANT=fxcp_headless PYTHON=python
```

The variant sets -m32 for compilation/linking and uses a 32-bit Windows resource.
No frozen modules or third-party SSL submodules are required. On the local
Windows machine the build used TDM-GCC and Git's sh/coreutils on PATH:

```powershell
$env:PATH = 'C:\Program Files\Git\usr\bin;' + $env:PATH
mingw32-make -j8 VARIANT=fxcp_headless PYTHON=python SHELL=sh
```

Output: build-fxcp_headless/micropython.exe. Use `-X heapsize=256k` to bound the
host heap. The checked-in fxcp-headless workflow builds and uploads the executable
and smoke-tests pointer width and tracing. It has not been dispatched remotely.

Supported: DWIDTH/DHEIGHT (320x528), C_BLACK/C_WHITE, symbolic LEFT/RIGHT/SHIFT/EXIT
keys, keydown, clearevents, pollevent, drect, dclear, dupdate, native buffer-backed
P4/P4A image construction and dsubimage bounds checks. Key numeric values are host
symbols, not the physical calculator key codes. Unsupported APIs raise normal
attribute errors; this cannot run arbitrary cpgame programs unchanged.

Extra instrumentation:

- trace_reset(): clear counters and input mask.
- trace_keys(mask): set input (left=1, right=2, shift=4, exit=8).
  Numeric-key aliases use 7=16 and 9=32 in the host mask.
  EXE/Enter uses 64 in the host mask.
- trace_touch(type, x, y): enqueue touch (down=1, drag=2, up=3). pollevent returns
  an attribute tuple; the bounded 64-event queue rejects overflow.
- trace_stats(): allocate (updates, drawing calls, clears, pixel writes).
  Calls include rectangles/subimages; bounds clip to the screen. Subimage counts
  include transparent bounding pixels. No framebuffer is allocated. Native
  image objects retain their Python buffer owners.

Reset counters for bounded runs; they are signed machine integers. The native
draw functions update C counters without allocating Python objects. Reading the
stats tuple and JSON serialization are tracing overhead and belong outside
allocation benchmarks. The host does not estimate SuperH cycles or LCD time.
