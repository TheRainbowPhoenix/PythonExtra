# Windows fxcp_headless

```sh
cd ports/windows
make -j2 VARIANT=fxcp_headless
build-fxcp_headless/micropython.exe -c "import gint; print(gint.trace_stats())"
```

Requires a MinGW 32-bit toolchain. Counter-only graphics instrumentation; no framebuffer.
