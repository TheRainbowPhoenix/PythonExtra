include ../../py/mkenv.mk

# Use the sh-elf toolchain
CROSS_COMPILE := sh-elf-

include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CFLAGS += -m4-nofpu -mb -fstrict-volatile-bitfields -I. -I$(BUILD) -I$(TOP) -DFXCG50 -O2 -Wall -Wextra -Wno-unused-parameter
LIBS += -nostdlib -Wl,--no-warn-rwx-segments -T fxcg50.ld -lm -lgint-cg -lc -lgint-cg -lgcc -Wl,-Map=build/map

# Source files
SRC_C = \
    main.c \
    console.c \
    keymap.c \
    mphalport.c \
    shared/readline/readline.c \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c \
    shared/runtime/stdout_helpers.c \

SRC_QSTR += \
    shared/readline/readline.c \

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

all: $(BUILD)/firmware.elf PythonExtra.g3a

PythonExtra.g3a: $(BUILD)/firmware.bin icon-uns.png icon-sel.png
	fxgxa --g3a -n PythonExtra --icon-uns=icon-uns.png --icon-sel=icon-sel.png $< -o $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(CC:gcc=objcopy) -O binary -R .bss -R .gint_bss $< $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

send: all
	fxlink -sw PythonExtra.g3a

include $(TOP)/py/mkrules.mk
