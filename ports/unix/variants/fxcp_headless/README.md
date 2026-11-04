# Linux fxcp_headless

```sh
sudo apt-get install -y build-essential gcc-multilib libc6-dev-i386 python3
make -C ports/unix -j2 VARIANT=fxcp_headless
ports/unix/build-fxcp_headless/micropython -c 'import gint; print(gint.trace_stats())'
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra
python3 tools/pe-headless/audit_examples.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra --out /tmp/fxcp-audit.json
```

Counter-only graphics instrumentation; no framebuffer. 32-bit Linux runtime required.
