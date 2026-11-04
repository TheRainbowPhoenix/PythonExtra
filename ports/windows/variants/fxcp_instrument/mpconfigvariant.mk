# Extend PythonExtra's existing Windows port; 32-bit GC geometry like fxcp.
CFLAGS_EXTRA += -m32 -DPE_HOST_INSTRUMENT=1
LDFLAGS_EXTRA += -m32
WINDRES += --target=pe-i386
FROZEN_MANIFEST =
