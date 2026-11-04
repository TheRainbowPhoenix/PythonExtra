# Keep pointers/GC blocks close to the 32-bit calculator. No optional SSL libs.
CFLAGS_EXTRA += -m32
LDFLAGS_EXTRA += -m32
FROZEN_MANIFEST =
WINDRES += --target=pe-i386
