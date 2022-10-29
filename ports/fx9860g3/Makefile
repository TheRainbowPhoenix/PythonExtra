include ../../py/mkenv.mk

SH_CFLAGS := -DFX9860G
SH_LDFLAGS := -T fx9860g.ld -lm -lgint-fx -lc -lgint-fx -lgcc

all: PythonExtra.g1a

PythonExtra.g1a: $(BUILD)/firmware.bin icon.png
	fxgxa --g1a -n PythonExtra -i icon.png $< -o $@

send: all
	fxlink -sw PythonExtra.g1a

include $(TOP)/ports/sh/Makefile
