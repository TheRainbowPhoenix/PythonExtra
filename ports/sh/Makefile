CROSS_COMPILE := sh-elf-

include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CFLAGS += -m4-nofpu -mb -fstrict-volatile-bitfields \
          -I. -I$(TOP)/ports/sh -I$(BUILD) -I$(TOP) \
          $(SH_CFLAGS) -Os -Wall -Wextra -Wno-unused-parameter
LIBS += -nostdlib -Wl,--no-warn-rwx-segments $(SH_LDFLAGS) -Wl,-Map=build/map

# Source files
SRC_C = \
    ports/sh/main.c \
    ports/sh/console.c \
    ports/sh/keymap.c \
    ports/sh/modcasioplot.c \
    ports/sh/modgint.c \
    ports/sh/modtime.c \
    ports/sh/mphalport.c \
    ports/sh/widget_shell.c \
    shared/readline/readline.c \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c \
    shared/runtime/stdout_helpers.c \
    shared/runtime/interrupt_char.c \

SRC_QSTR += \
    shared/readline/readline.c \
    shared/runtime/pyexec.c \
    ports/sh/main.c \
    ports/sh/modcasioplot.c \
    ports/sh/modgint.c \
    ports/sh/modtime.c \

ASSETS_O := $(SH_ASSETS:%=$(BUILD)/sh_assets/%.o)

OBJ = $(PY_O) $(ASSETS_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(CC:gcc=objcopy) -O binary -R .bss -R .gint_bss $< $@

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/sh_assets/%.o: % $(SH_METADATA)
	$(Q)mkdir -p $(dir $@)
	$(Q)fxconv --toolchain=$(CROSS_COMPILE:%-=%) $(SH_CONVFLAGS) $< -o $@

include $(TOP)/py/mkrules.mk
