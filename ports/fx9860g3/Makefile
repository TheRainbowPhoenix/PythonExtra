include ../../py/mkenv.mk

SH_CFLAGS := -DFX9860G
SH_LDFLAGS := -T fx9860g.ld -ljustui-fx -lm -lgint-fx -lc -lgint-fx -lgcc

SH_ASSETS := img_fkeys_main.png font_5x7.png font_4x4.png font_4x6.png
SH_METADATA := fxconv-metadata.txt
SH_CONVFLAGS := --fx

all: PythonExtra.g1a

PythonExtra.g1a: $(BUILD)/firmware.bin icon.png
	fxgxa --g1a -n PythonExtra -i icon.png $< -o $@

send: all
	fxlink -sw PythonExtra.g1a

include $(TOP)/ports/sh/Makefile
