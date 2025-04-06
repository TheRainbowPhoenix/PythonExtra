include ../../py/mkenv.mk

SH_CFLAGS := -DFXCP
TARGETCASIO := "FXCP"
SH_LDFLAGS := -T fxcp_hh2.ld -ljustui-cp -lm -lgint-cp -lc -lgint-cp -lgcc

SH_ASSETS := img_modifier_states.png font_9.png font_13.png font_19.png
SH_METADATA := fxconv-metadata.txt
SH_CONVFLAGS := --cg

all: PythonExtra-hh2.bin

ICONS_RELEASE := --icon-uns=icon-uns.png --icon-sel=icon-sel.png
ICONS_DEV := --icon-uns=icon-uns-dev.png --icon-sel=icon-sel-dev.png

PythonExtra-hh2.bin: $(BUILD)/firmware.elf
	cp $< $(BUILD)/firmware-hh2.elf
	fxsdk script patch_hh2_filename.py $(BUILD)/firmware-hh2.elf $@
	sh-elf-objcopy -O binary -R .bss -R .gint_bss $(BUILD)/firmware-hh2.elf $@

send: all
	fxlink -sw PythonExtra-hh2.bin

.PRECIOUS: $(BUILD)/firmware-hh2.elf

include $(TOP)/ports/sh/Makefile
