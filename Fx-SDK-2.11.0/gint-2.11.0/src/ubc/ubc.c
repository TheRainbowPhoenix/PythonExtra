#include <gint/drivers.h>
#include <gint/exc.h>
#include <gint/gdb.h>
#include <gint/mpu/power.h>
#include <gint/mpu/ubc.h>
#include <gint/ubc.h>

#define UBC   SH7305_UBC
#define POWER SH7305_POWER

static bool hpowered(void)
{
	return POWER.MSTPCR0.UDB == 0;
}

static void hpoweron(void)
{
	// Power on the UBC via MSTPCR0
	POWER.MSTPCR0.UDB = 0;

	// Set the DBR register
	ubc_setDBR(ubc_dbh);

	// Disable break channel 0 and 1
	UBC.CBR0.CE = 0;
	UBC.CBR1.CE = 0;

	// Enable user break debugging support (i.e. usage of the DBR register)
	UBC.CBCR.UBDE = 1;
}

static void hpoweroff(void)
{
	POWER.MSTPCR0.UDB = 1;

	UBC.CBR0.CE = 0;
	UBC.CBR1.CE = 0;
}

#define UBC_BREAK_CHANNEL(CBR, CRR, CAR, CAMR) do { \
	UBC.CBR.MFE = 0; /* Don't include Match Flag in match condition */         \
	UBC.CBR.AIE = 0; /* Don't include ASID check in match condition */         \
	UBC.CBR.MFI = 0; /* Default value of MFI is reserved, make it legal */     \
	UBC.CBR.SZ  = 0; /* Match on any operand size */                           \
	UBC.CBR.CD  = 0; /* Match on operand bus access */                         \
	UBC.CBR.ID  = 1; /* Match on instruction fetch */                          \
	UBC.CBR.RW  = 1; /* Match on read cycle */                                 \
                                                                                   \
	UBC.CRR.PCB = pcb; /* Set PC break {before,after} instruction execution */ \
	UBC.CRR.BIE = 1;   /* Break when channel matches */                        \
                                                                                   \
	UBC.CAR = (uint32_t)break_address; /* Match address */                     \
	UBC.CAMR = 0;                      /* Match mask (0 bits are included) */  \
	UBC.CBR.CE = 1;                    /* Enable channel */                    \
} while (0)
bool ubc_set_breakpoint(int channel, void* break_address, ubc_break_mode_t break_mode)
{
	uint32_t pcb = break_mode == UBC_BREAK_AFTER ? 1 : 0;
	if (channel == 0) {
		UBC_BREAK_CHANNEL(CBR0, CRR0, CAR0, CAMR0);
		return true;
	} else if (channel == 1) {
		UBC_BREAK_CHANNEL(CBR1, CRR1, CAR1, CAMR1);
		return true;
	} else {
		return false;
	}
}
#undef UBC_BREAK_CHANNEL

bool ubc_get_break_address(int channel, void** break_address)
{
	if (channel == 0 && UBC.CBR0.CE) {
		*break_address = (void*) UBC.CAR0;
		return true;
	} else if (channel == 1 && UBC.CBR1.CE) {
		*break_address = (void*) UBC.CAR1;
		return true;
	} else {
		return false;
	}
}

bool ubc_disable_channel(int channel)
{
	if (channel == 0) {
		UBC.CBR0.CE = 0;
		return true;
	} else if (channel == 1) {
		UBC.CBR1.CE = 0;
		return true;
	} else {
		return false;
	}
}

static void (*ubc_application_debug_handler)(gdb_cpu_state_t*) = NULL;
void ubc_debug_handler(gdb_cpu_state_t* cpu_state)
{
	// Clear match flags
	UBC.CCMFR.lword = 0;

	if (ubc_application_debug_handler != NULL) {
		ubc_application_debug_handler(cpu_state);
	}
	// For now we will ignore breaks when no debug handler is set
	// TODO : Should we log or panic when no debug handler is set ?
}

// TODO : Should we use the struct designed for GDB or make it more generic ?
void ubc_set_debug_handler(void (*h)(gdb_cpu_state_t*)) {
	ubc_application_debug_handler = h;
}

gint_driver_t drv_ubc = {
	.name      = "UBC",
	.hpowered  = hpowered,
	.hpoweron  = hpoweron,
	.hpoweroff = hpoweroff,
	.flags     = GINT_DRV_SHARED,
};
GINT_DECLARE_DRIVER(30, drv_ubc);
