include ../../py/mkenv.mk

SH_CFLAGS := -DFXCG50
TARGETCASIO := "FXCG50"
SH_LDFLAGS := -T fxcg50.ld -ljustui-cg -lm -lgint-cg -lc -lgint-cg -lgcc

SH_ASSETS := img_modifier_states.png font_9.png font_13.png font_19.png PoliceNW.png
SH_METADATA := fxconv-metadata.txt
SH_CONVFLAGS := --cg
 
all: PythonExtra.g3a

PythonExtra.g3a: $(BUILD)/firmware.bin icon-uns.png icon-sel.png
	fxgxa --g3a -n PyExtra_NW --icon-uns=icon-uns.png --icon-sel=icon-sel.png $< -o $@

send: all
	fxlink -sw PythonExtra.g3a

include $(TOP)/ports/sh/Makefile
