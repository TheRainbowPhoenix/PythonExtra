//---
// gint:cpu - Driver for CPU built-in features
//---

#include <gint/cpu.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/hardware.h>

/* VBR address to be used in the next world's configure() */
static uint32_t configure_VBR = 0;

void cpu_configure_VBR(uint32_t VBR)
{
	configure_VBR = VBR;
}

static void configure(void)
{
	cpu_setVBR(configure_VBR);
	configure_VBR = 0;

	if(isSH4()) {
		/* Set CPUOPM.INTMU. On the fx-CG 50 emulator it is available but
		   ignored by the emulator, so additional checks still need to be done
		   in interrupt handlers. */
		cpu_setCPUOPM(cpu_getCPUOPM() | 0x00000008);

		/* Enable DSP instructions */
		cpu_sr_t SR = cpu_getSR();
		SR.DSP = 1;
		cpu_setSR(SR);
	}
}

//---
// Device state and driver metadata
//---

static void hsave(cpu_state_t *s)
{
	s->VBR = cpu_getVBR();

	if(isSH4()) {
		s->CPUOPM = cpu_getCPUOPM();
		s->SR = cpu_getSR().lword;
	}

	__asm__("stc r0_bank, %0" : "=r"(s->rN_bank[0]));
	__asm__("stc r1_bank, %0" : "=r"(s->rN_bank[1]));
	__asm__("stc r2_bank, %0" : "=r"(s->rN_bank[2]));
	__asm__("stc r3_bank, %0" : "=r"(s->rN_bank[3]));
	__asm__("stc r4_bank, %0" : "=r"(s->rN_bank[4]));
	__asm__("stc r5_bank, %0" : "=r"(s->rN_bank[5]));
	__asm__("stc r6_bank, %0" : "=r"(s->rN_bank[6]));
	__asm__("stc r7_bank, %0" : "=r"(s->rN_bank[7]));
}

static void hrestore(cpu_state_t const *s)
{
	cpu_setVBR(s->VBR);

	if(isSH4()) {
		cpu_setCPUOPM(s->CPUOPM);
		cpu_setSR((cpu_sr_t)s->SR);
	}

	__asm__("ldc %0, r0_bank" :: "r"(s->rN_bank[0]));
	__asm__("ldc %0, r1_bank" :: "r"(s->rN_bank[1]));
	__asm__("ldc %0, r2_bank" :: "r"(s->rN_bank[2]));
	__asm__("ldc %0, r3_bank" :: "r"(s->rN_bank[3]));
	__asm__("ldc %0, r4_bank" :: "r"(s->rN_bank[4]));
	__asm__("ldc %0, r5_bank" :: "r"(s->rN_bank[5]));
	__asm__("ldc %0, r6_bank" :: "r"(s->rN_bank[6]));
	__asm__("ldc %0, r7_bank" :: "r"(s->rN_bank[7]));
}

gint_driver_t drv_cpu = {
	.name        = "CPU",
	.configure   = configure,
	.hsave       = (void *)hsave,
	.hrestore    = (void *)hrestore,
	.state_size  = sizeof(cpu_state_t),
};
GINT_DECLARE_DRIVER(00, drv_cpu);
