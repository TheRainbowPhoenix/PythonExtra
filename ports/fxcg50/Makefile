include ../../py/mkenv.mk

SH_CFLAGS := -DFXCG50
SH_LDFLAGS := -T fxcg50.ld -lm -lgint-cg -lc -lgint-cg -lgcc

all: PythonExtra.g3a

PythonExtra.g3a: $(BUILD)/firmware.bin icon-uns.png icon-sel.png
	fxgxa --g3a -n PythonExtra --icon-uns=icon-uns.png --icon-sel=icon-sel.png $< -o $@

send: all
	fxlink -sw PythonExtra.g3a

include $(TOP)/ports/sh/Makefile
