include ../../py/mkenv.mk

SH_CFLAGS := -DFXCP
TARGETCASIO := "FXCP"
SH_LDFLAGS := -T fxcp_hh3.ld -ljustui-cp -lm -lgint-cp -lc -lgint-cp -lgcc

SH_ASSETS := img_modifier_states.png font_9.png font_13.png font_19.png
SH_METADATA := fxconv-metadata.txt
SH_CONVFLAGS := --cg

all: PythonExtra.hh3

ICONS_RELEASE := --icon-uns=icon-uns.png --icon-sel=icon-sel.png
ICONS_DEV := --icon-uns=icon-uns-dev.png --icon-sel=icon-sel-dev.png

PythonExtra.hh3: $(BUILD)/firmware.elf
	sh-elf-strip -s -o $@ $<

send: all
	fxlink -sw PythonExtra.hh3

include $(TOP)/ports/sh/Makefile
