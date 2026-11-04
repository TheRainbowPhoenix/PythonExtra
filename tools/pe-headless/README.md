# fxcp_headless tests

```sh
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra --json /tmp/probe.json
python3 tools/pe-headless/audit_examples.py --exe ports/unix/build-fxcp_headless/micropython --examples ../MiscPythonExtra --out /tmp/audit.json
python3 tools/pe-headless/probe.py --exe ports/unix/build-fxcp_headless/micropython --program ../MiscPythonExtra/bounce.py
```

The small Linux/Windows `fxcp_headless` counts calls but cannot display frames.
`view.py --debug` belongs to the separate Windows `fxcp_instrument` host.
