# Linux fxcp_headless

32-bit Linux MicroPython interpreter with the same native, deliberately partial
`gint` tracing API as `ports/windows/variants/fxcp_headless`.

From the repository root on Ubuntu/Debian x86_64:

```sh
sudo apt-get install build-essential gcc-multilib libc6-dev-i386 python3
make -C ports/unix -j2 VARIANT=fxcp_headless
ports/unix/build-fxcp_headless/micropython -c 'import gint; print(gint.trace_stats())'
```

Output: `ports/unix/build-fxcp_headless/micropython` (32-bit ELF i386).
On 64-bit Linux hosts, executing the ELF requires 32-bit glibc runtime support
(e.g. `libc6-i386` on Ubuntu). The executable is *not* a static binary.

The `gint` API, bounds checks, event queue, and draw counters are reused
directly from the Windows variant to prevent platform drift. This is not a
SuperH or full calculator emulator. Unsupported `gint` APIs raise
`AttributeError`. For instrumentation methods, consult the Windows variant's
README.

This variant intentionally omits optional SSL, FFI, sockets, threads and
frozen modules. It does not include the larger Windows-only
`fxcp_instrument` host (Win32 mapping/rendering/USB implementation).
