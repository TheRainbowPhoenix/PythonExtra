//---
// gint:mpu:ubc - User Break Controller
//---

#ifndef GINT_MPU_UBC
#define GINT_MPU_UBC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

typedef volatile struct
{
	lword_union(CBR0, /* Match condition setting 0 */
		uint32_t MFE	:1; /* Match Flag Enable */
		uint32_t AIE	:1; /* ASID Enable */
		uint32_t MFI	:6; /* Match Flag Specify */
		uint32_t AIV	:8; /* ASID Specify */
		uint32_t	:1;
		uint32_t SZ	:3; /* Operand Size Select */
		uint32_t	:4;
		uint32_t CD	:2; /* Bus Select */
		uint32_t ID	:2; /* Ins. Fetch / Operand Access Select */
		uint32_t	:1;
		uint32_t RW	:2; /* Bus Command Select */
		uint32_t CE	:1; /* Channel Enable */
	);
	lword_union(CRR0, /* Match operation setting 0 */
		uint32_t	:18;
		uint32_t _1	:1; /* Always set to 1 */
		uint32_t	:11;
		uint32_t PCB	:1; /* PC Break Select */
		uint32_t BIE	:1; /* Break Enable */
	);
	uint32_t CAR0; /* Match address setting 0 */
	uint32_t CAMR0; /* Match address mask setting 0 */
	pad(0x10);

	lword_union(CBR1, /* Match condition setting 1 */
		uint32_t MFE	:1; /* Match Flag Enable */
		uint32_t AIE	:1; /* ASID Enable */
		uint32_t MFI	:6; /* Match Flag Specify */
		uint32_t AIV	:8; /* ASID Specify */
		uint32_t DBE	:1; /* Data Value Enable */
		uint32_t SZ	:3; /* Operand Size Select */
		uint32_t ETBE	:1; /* Execution Count Value Enable */
		uint32_t	:3;
		uint32_t CD	:2; /* Bus Select */
		uint32_t ID	:2; /* Ins. Fetch / Operand Access Select */
		uint32_t	:1;
		uint32_t RW	:2; /* Bus Command Select */
		uint32_t CE	:1; /* Channel Enable */
	);
	lword_union(CRR1, /* Match operation setting 1 */
		uint32_t	:18;
		uint32_t _1	:1; /* Always set to 1 */
		uint32_t	:11;
		uint32_t PCB	:1; /* PC Break Select */
		uint32_t BIE	:1; /* Break Enable */
	);
	uint32_t CAR1; /* Match address setting 1 */
	uint32_t CAMR1; /* Match address mask setting 1 */
	uint32_t CDR1; /* Match data setting 1 */
	uint32_t CDMR1; /* Match data mask setting 1 */
	lword_union(CETR1, /* Execution count break 1 */
		uint32_t	:20;
		uint32_t CET	:12; /* Execution Count */
	);
	pad(0x5c4);

	lword_union(CCMFR, /* Channel match flag */
		uint32_t	:30;
		uint32_t MF1	:1; /* Channel 1 Condition Match Flag */
		uint32_t MF0	:1; /* Channel 0 Condition Match Flag */
	);
	pad(0x1c);
	lword_union(CBCR, /* Break control */
		uint32_t	:31;
		uint32_t UBDE	:1; /* User Break Debugging Support Function Enable */
	);
} GPACKED(4) sh7305_ubc_t;
#define SH7305_UBC (*(sh7305_ubc_t *)0xff200000)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_UBC */
