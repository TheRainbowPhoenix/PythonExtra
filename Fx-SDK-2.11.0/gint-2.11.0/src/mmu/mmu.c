//---
//	gint:mmu:mmu - MMU driver definition and context management
//---

#include <gint/mmu.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/hardware.h>

//---
//	Unified interface
//---

/* mmu_translate(): Get the physical address for a virtual page */
uint32_t mmu_translate(uint32_t page, uint32_t *size)
{
	return isSH3() ? tlb_translate(page,size) : utlb_translate(page,size);
}

/* mmu_uram(): Get pointer to physical start of user RAM */
void *mmu_uram(void)
{
	/* Use P1 access */
	return (void *)(mmu_translate(0x08100000, NULL) | 0x80000000);
}

/* mmu_uram_size(): Get size of user RAM area */
uint32_t mmu_uram_size(void)
{
	uint32_t size = 0;
	uint32_t pagesize;

	while(mmu_translate(0x08100000 + size, &pagesize) != (uint32_t)-1)
	{
		size += pagesize;
	}

	return size;
}

/* mmu_is_rom(): Determine if an address points to ROM */
bool mmu_is_rom(void const *ptr)
{
	uint32_t a = (uint32_t)ptr;

	if(a >= 0x80000000 && a < 0x88000000)
		return true;
	if(a >= 0xa0000000 && a < 0xa8000000)
		return true;
	if(a >= 0x00300000 && a < 0x00800000)
		return true;

	return false;
}

//---
//	SH7705 TLB
//---

#if GINT_HW_FX

/* tlb_addr() - get the P4 address of a TLB address entry */
GINLINE const tlb_addr_t *tlb_addr(uint way, uint E)
{
	uint32_t addr = 0xf2000000 | (E << 12) | (way << 8);
	return (void *)addr;
}

/* tlb_data() - get the P4 address of a TLB data entry */
GINLINE const tlb_data_t *tlb_data(uint way, uint E)
{
	uint32_t addr = 0xf3000000 | (E << 12) | (way << 8);
	return (void *)addr;
}

/* tlb_mapped_memory() - count amount of mapped memory */
void tlb_mapped_memory(uint32_t *p_rom, uint32_t *p_ram)
{
	uint32_t rom = 0, ram = 0;

	for(int way = 0; way < 4; way++)
	for(int E = 0; E < 32; E++)
	{
		const tlb_addr_t *addr = tlb_addr(way, E);
		const tlb_data_t *data = tlb_data(way, E);
		if(!addr->V || !data->V) continue;

		int size = data->SZ ? 4096 : 1024;
		uint32_t src;

		if(data->SZ) src = (((addr->VPN >> 2) | E) << 12);
		else src = (addr->VPN | (E << 2)) << 10;

		if(src >= 0x00300000 && src < 0x00380000) rom += size;
		if(src >= 0x08100000 && src < 0x08180000) ram += size;
	}

	if(p_rom) *p_rom = rom;
	if(p_ram) *p_ram = ram;

	gint[HWURAM] = ram;
}

/* tlb_translate(): Get the physical address for a virtual page */
uint32_t tlb_translate(uint32_t page, uint32_t *size)
{
	for(int way = 0; way < 4; way++)
	for(int E = 0; E < 32; E++)
	{
		const tlb_addr_t *addr = tlb_addr(way, E);
		const tlb_data_t *data = tlb_data(way, E);
		if(!addr->V || !data->V) continue;

		uint32_t src;
		if(data->SZ) src = (((addr->VPN >> 2) | E) << 12);
		else src = (addr->VPN | (E << 2)) << 10;

		if(src == page)
		{
			if(size) *size = (data->SZ ? 4096 : 1024);
			return data->PPN << 10;
		}
	}
	return -1;
}
#endif

//---
//	SH7305 Unified TLB
//---

GINLINE const utlb_addr_t *utlb_addr(uint E)
{
	uint32_t addr = 0xf6000000 | ((E & 0x3f) << 8);
	return (void *)addr;
}

GINLINE const utlb_data_t *utlb_data(uint E)
{
	uint32_t addr = 0xf7000000 | ((E & 0x3f) << 8);
	return (void *)addr;
}

void utlb_mapped_memory(uint32_t *p_rom, uint32_t *p_ram)
{
	uint32_t rom = 0, ram = 0;

	for(int E = 0; E < 64; E++)
	{
		const utlb_addr_t *addr = utlb_addr(E);
		const utlb_data_t *data = utlb_data(E);
		if(!addr->V || !data->V) continue;

		/* Magic formula to get the size without using an array since
		   this code is used even before global data is initialized */
		int sz = ((data->SZ1 << 1) | data->SZ2) << 3;
		int size = 1 << ((0x14100c0a >> sz) & 0xff);

		uint32_t src = addr->VPN << 10;
		if(src >= 0x00300000 && src < 0x00700000) rom += size;
		if(src >= 0x08100000 && src < 0x08180000) ram += size;
	}

	if(p_rom) *p_rom = rom;
	if(p_ram) *p_ram = ram;

	gint[HWURAM] = ram;
}

uint32_t utlb_translate(uint32_t page, uint32_t *size)
{
	for(int E = 0; E < 64; E++)
	{
		const utlb_addr_t *addr = utlb_addr(E);
		const utlb_data_t *data = utlb_data(E);
		if(!addr->V || !data->V) continue;

		if((uint32_t)addr->VPN << 10 == page)
		{
			/* Same magic formula as utlb_mapped_memory() */
			int sz = ((data->SZ1 << 1) | data->SZ2) << 3;
			if(size) *size = 1 << ((0x14100c0a >> sz) & 0xff);

			return data->PPN << 10;
		}
	}
	return -1;
}

itlb_addr_t const *itlb_addr(uint E)
{
	uint32_t addr = 0xf2000000 | ((E & 3) << 8);
	return (void *)addr;
}

itlb_data_t const *itlb_data(uint E)
{
	uint32_t addr = 0xf3000000 | ((E & 3) << 8);
	return (void *)addr;
}

static void configure(void)
{
	/* Make writes to the control register area synchronous; this is needed
	   for the SPU to operate properly */
	if(isSH4()) SH7305_MMU.PASCR.UBC = 1;
}

//---
// State and driver metadata
//---

static void hsave(mmu_state_t *s)
{
	if(isSH3()) return;
	s->PASCR = SH7305_MMU.PASCR.lword;
	s->IRMCR = SH7305_MMU.IRMCR.lword;
}

static void hrestore(mmu_state_t const *s)
{
	if(isSH3()) return;
	SH7305_MMU.PASCR.lword = s->PASCR;
	SH7305_MMU.IRMCR.lword = s->IRMCR;
}

gint_driver_t drv_mmu = {
	.name         = "MMU",
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(mmu_state_t),
};
GINT_DECLARE_DRIVER(02, drv_mmu);
