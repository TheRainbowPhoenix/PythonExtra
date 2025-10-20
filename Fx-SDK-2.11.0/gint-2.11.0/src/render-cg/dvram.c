#include <gint/display.h>
#include <gint/kmalloc.h>
#include <gint/video.h>
#include <gint/image.h>
#include <gint/config.h>
#include <string.h>
#include <stdlib.h>
#if GINT_RENDER_RGB

#if GINT_OS_CP

extern void *__GetVRAMAddress(void);

uint16_t *gint_vram = NULL;

static uint8_t *gint_vrambackup = NULL;
static int gint_vrambackup_size = -1;

bool dvram_init(void)
{
	/* Backup the VRAM up, but not to the normal backup area--we use that
	   to load code. Instead, save over VRAM itself then copy to heap. */
	void *VRAM      = (void *)0x8c000000;
	void *VRAM_END  = (void *)0x8c052800;
	void *SCRATCH   = VRAM;

	// prof_enter(*p1);
	void *SCRATCH_END = gint_vrambackup_encode(SCRATCH, VRAM, VRAM_END);
	// prof_leave(*p1);

	// prof_enter(*p2);
	gint_vrambackup_size = (u8 *)SCRATCH_END - (u8 *)SCRATCH;
	gint_vrambackup = malloc(gint_vrambackup_size);
	if(gint_vrambackup)
		memcpy(gint_vrambackup, SCRATCH, gint_vrambackup_size);
	// prof_leave(*p2);

	gint_vram = __GetVRAMAddress();
	return true;
}

void dvram_quit(void)
{
	// TODO: CP dvram_quit: use global framebuffer image
	image_t *img = image_create_vram();
	gint_vrambackup_show();
	free(gint_vrambackup);
	gint_vrambackup = NULL;
	video_update(0, 0, img, VIDEO_UPDATE_FOREIGN_WORLD);
	image_free(img);
}

void dgetvram(uint16_t **ptr_vram_1, uint16_t **ptr_vram_2)
{
	*ptr_vram_1 = *ptr_vram_2 = gint_vram;
}

void gint_vrambackup_show(void)
{
	uint8_t *rle = gint_vrambackup;
	int i = 0;
	while(i < DWIDTH * DHEIGHT) {
		int index = *rle++;
		int run_length, run_color;

		if(index >= 110) {
			run_length = 1;
			run_color = gint_vrambackup_palette[index - 110];
		}
		else {
			run_length = *rle++;
			run_color = gint_vrambackup_palette[index];
		}

		for(int j = 0; j < run_length; j++)
			gint_vram[i+j] = run_color;
		i += run_length;
	}
}

void *gint_vrambackup_get(int *size)
{
	if(size)
		*size = gint_vrambackup_size;
	return gint_vrambackup;
}

#elif GINT_OS_CG
// TODO[3]: CG: Remove triple buffering

/* Up to two VRAM pointers can be set, for triple buffering. */
static uint16_t *vram_1 = NULL, *vram_2 = NULL;
/* Current VRAM pointer, always equal to either vram_1 or vram_2. */
uint16_t *gint_vram = NULL;

bool dvram_init(void)
{
	int const MARGIN = 32;

	/* Leave MARGIN bytes on each side of the region; this enables some
	   important optimizations in the image renderer. We also add another
	   32 bytes so we can manually 32-align the region */
	uint32_t region = (uint32_t)kmalloc(DWIDTH*DHEIGHT*2 + MARGIN*2 + 32,
#if !defined(GINT_NO_OS_STACK)
		"_ostk"
#else
		NULL
#endif
	);
	if(region == 0)
		return false;

	/* 32-align the region */
	region = (region + 31) & -32;
	/* Skip a MARGIN */
	region += MARGIN;
	/* Use an uncached address */
	region = (region & 0x1fffffff) | 0xa0000000;

	/* Don't enable triple buffering by default */
	vram_1 = (void *)region;
	vram_2 = vram_1;
	gint_vram = vram_1;
	return true;
}

/* dsetvram(): Control video RAM address and triple buffering */
void dsetvram(uint16_t *new_vram_1, uint16_t *new_vram_2)
{
	if(!new_vram_1 && !new_vram_2) return;
	if(!new_vram_1) new_vram_1 = new_vram_2;
	if(!new_vram_2) new_vram_2 = new_vram_1;

	if(gint_vram == vram_1)
		gint_vram = new_vram_1;
	else if(gint_vram == vram_2)
		gint_vram = new_vram_2;

	vram_1 = new_vram_1;
	vram_2 = new_vram_2;
}

/* dgetvram(): Get VRAM addresses */
void dgetvram(uint16_t **ptr_vram_1, uint16_t **ptr_vram_2)
{
	if(ptr_vram_1) *ptr_vram_1 = vram_1;
	if(ptr_vram_2) *ptr_vram_2 = vram_2;
}

/* dvram_switch(): Triple buffering switch
   This function is not part of the API; it is used only by dupdate(). */
void dvram_switch(void)
{
	gint_vram = (gint_vram == vram_1) ? vram_2 : vram_1;
}

void dvram_quit(void)
{
}

#endif /* OS type */
#endif /* GINT_RENDER_RGB */
