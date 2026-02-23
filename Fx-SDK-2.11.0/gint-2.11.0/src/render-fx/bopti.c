#include <gint/defs/types.h>
#include <gint/defs/util.h>
#include <gint/display.h>
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

#pragma GCC optimize("O3")

/* struct command: A rendering command
   Includes many computed parameters and handy information. Read-only. */
struct command
{
	/* x-coordinate of rendering box & 31, used for shifts */
	int x;
	/* VRAM pointers */
	uint32_t *v1;
	uint32_t *v2;
	/* Initial offset into VRAM */
	int offset;

	/* Number of VRAM columns affected by the bounding box; this is the
	   same as the number of rendered image columns if x=0, and this number
	   plus 1 otherwise. */
	int columns;
	/* A certain set of rendering masks (see bopti_render()) */
	uint32_t *masks;
	/* Whether the first column is real (ie. x>=0) or not */
	int real_start;
	/* Whether the last column is written to VRAM */
	int real_end;

	/* Ignored elements between two rendered grid rows */
	int vram_stride;
	/* Ignored elements between two rendered grid columns */
	int data_stride;

	/* Whether the image should be drawn on gray mode (this may be 1 even
	   for images of the mono and mono_alpha profiles) */
	int gray;

	/* Assembly function, prototype depends on image type */
	bopti_asm_t f;
};


/* List of rendering functions */
static asm_mono_t *asm_mono[] = {
	bopti_asm_mono,
	bopti_asm_mono_alpha,
};
static asm_gray_t *asm_gray[] = {
	bopti_gasm_mono,
	bopti_gasm_mono_alpha,
	bopti_gasm_gray,
	bopti_gasm_gray_alpha,
};
static asm_mono_scsp_t *asm_mono_scsp[] = {
	bopti_asm_mono_scsp,
	bopti_asm_mono_alpha_scsp,
};
static asm_gray_scsp_t *asm_gray_scsp[] = {
	bopti_gasm_mono_scsp,
	bopti_gasm_mono_alpha_scsp,
	bopti_gasm_gray_scsp,
	bopti_gasm_gray_alpha_scsp,
};

void bopti_grid(void **layer, int rows, struct command *c)
{
	/* Pointers to vram data */
	uint32_t *v1 = c->v1, *v2 = c->v2;
	/* Current offset into video RAM */
	uint offset = c->offset;
	/* Pairs of VRAM operands. A function that returns such a pair will be
	   optimized by GCC into a function returning into r0,r1 which will
	   avoid some memory accesses. */
	pair_t  p, pret = { 0 };
	/* Same with two pairs for the gray version (no optimization here) */
	quadr_t q, qret = { 0 };

	/* Monochrome version */
	if(!c->gray) while(rows--)
	{
		p.r = pret.r = v1[offset & 0xff];

		for(int col = 0; col < c->columns; col++)
		{
			/* Shift the pair to the left. When x=0, we should have
			   pret.r = p.r but due to some intentional UB with
			   32-bit shifts, pret.r != p.r so we reload p.r. */
			p.l = (c->x) ? pret.r : p.r;
			/* Load new second element, if offset+1 overflows from
			   the VRAM we load from offset 0. It doesn't matter
			   because the result will not be written back, I just
			   want to avoid reading from outside the VRAM. */
			p.r = v1[(offset + 1) & 0xff];

			/* The assembly routine blends a longword of data onto
			   the pair and returns the resulting pair. */
			pret = c->f.asm_mono(p, layer, c->masks+col+col,-c->x);

			/* Write back the result into VRAM, except for column
			   -1 (occurs once every row, iff visual_x < 0) */
			if(c->real_start + col) v1[offset] = pret.l;

			offset++;
		}

		if(c->real_end) v1[offset] = pret.r;

		*layer += c->data_stride;
		offset += c->vram_stride;
	}

	/* Gray version */
	else while(rows--)
	{
		if(c->real_start)
		{
			q.r1 = qret.r1 = v1[offset & 0xff];
			q.r2 = qret.r2 = v2[offset & 0xff];
		}

		/* Same as before, but 2 buffers at the same time */
		for(int col = 0; col < c->columns; col++)
		{
			q.l1 = (c->x) ? qret.r1 : q.r1;
			q.r1 = v1[(offset + 1) & 0xff];
			q.l2 = (c->x) ? qret.r2 : q.r2;
			q.r2 = v2[(offset + 1) & 0xff];

			c->f.asm_gray(q, layer, c->masks+col+col, -c->x,&qret);

			if(c->real_start + col)
			{
				v1[offset] = qret.l1;
				v2[offset] = qret.l2;
			}

			offset++;
		}

		if(c->real_end)
		{
			v1[offset] = qret.r1;
			v2[offset] = qret.r2;
		}

		*layer += c->data_stride;
		offset += c->vram_stride;
	}
}

void bopti_render(bopti_image_t const *img, struct rbox *rbox, uint32_t *v1,
	uint32_t *v2)
{
	/* Rendering function */
	bopti_asm_t f;
	if(v2) f.asm_gray = asm_gray[img->profile];
	else   f.asm_mono = asm_mono[img->profile];

	/* Compute rendering masks */
	uint32_t vm[4];
	masks(rbox->visual_x, rbox->visual_x + rbox->width - 1, vm);

	/* Number of layers per profile */
	int layers = image_layer_count(img->profile);

	/* For each pair of consecutive VRAM elements involved, create a mask
	   from the intersection of the standard vram mask with the shift-mask
	   related to x not being a multiple of 32 */
	uint32_t masks[10] = {
		    0, vm[0],
		vm[0], vm[1],
		vm[1], vm[2],
		vm[2], vm[3],
		vm[3],     0,
	};

	uint32_t mx = 0xffffffff >> (rbox->x & 31);
	for(int i = 0; i < 5; i++)
	{
		masks[2*i]   &= mx;
		masks[2*i+1] &= ~mx;
	}

	/* Position, in masks[], of the first column being rendered */
	int left_origin = (rbox->x >> 5) + 1;

	/* Number of columns in [img] */
	int img_columns = (img->width + 31) >> 5;

	/* Interwoven layer data. Skip left columns that are not rendered */
	const uint32_t *layer = (void *)img->data;
	layer += (rbox->top * img_columns + rbox->left) * layers;

	/* Compute and execute the command for this parameters */
	struct command c = {
	  .x            = rbox->x & 31,
	  .v1           = v1,
	  .v2           = v2 ? v2 : v1,
	  .offset       = (rbox->y << 2) + (rbox->x >> 5),
	  .columns      = rbox->columns,
	  .masks        = masks + 2 * left_origin,
	  .real_start   = (left_origin > 0),
	  .real_end     = (rbox->x & 31) && (left_origin + rbox->columns < 5),
	  .vram_stride  = 4 - rbox->columns,
	  .data_stride  = ((img_columns - rbox->columns) << 2) * layers,
	  .gray         = (v2 != NULL),
	  .f            = f,
	};
	bopti_grid((void **)&layer, rbox->height, &c);
}

/* Specialized, faster version for single-column single-position instances */
void bopti_render_scsp(bopti_image_t const *img, struct rbox *rbox,
	uint32_t *v1, uint32_t *v2)
{
	/* Compute the only rendering mask */
	uint32_t mask =
		(0xffffffff << (32 - rbox->width)) >> (rbox->visual_x & 31);

	/* Number of layers */
	int layers = image_layer_count(img->profile);

	/* Number of longwords to skip between rows of [img] */
	int img_stride = ((img->width + 31) >> 5) * layers;

	/* Interwoven layer data. Skip left columns that are not rendered */
	const uint32_t *layer = (void *)img->data;
	layer += (rbox->top * img_stride) + (rbox->left * layers);

	/* Starting value of VRAM pointers */
	int offset = (rbox->y << 2) + (rbox->visual_x >> 5);
	v1 += offset;

	/* Number of rows */
	int rows = rbox->height;

	/* Render the grid immediately; mono version */
	if(!v2)
	{
		asm_mono_scsp_t *f = asm_mono_scsp[img->profile];
		while(rows--)
		{
			f(v1, layer, mask, rbox->x);
			layer += img_stride;
			v1 += 4;
		}
	}
	/* Gray version */
	else
	{
		asm_gray_scsp_t *f = asm_gray_scsp[img->profile];
		v2 += offset;

		while(rows--)
		{
			f(v1, layer, mask, v2, rbox->x);
			layer += img_stride;
			v1 += 4;
			v2 += 4;
		}
	}
}

int bopti_clip(bopti_image_t const *img, struct rbox *r)
{
	/* This load/save is not elegant but it makes GCC use register-only
	   operations, which is what we need for efficiency */
	int x = r->visual_x, y = r->y;
	int left = r->left, top = r->top;
	int width = r->width, height = r->height;
	int diff;

	/* Adjust the bounding box of the input image */
	if(left < 0) width  += left, x -= left, left = 0;
	if(top  < 0) height += top,  y -= top,  top  = 0;
	width = min(width, img->width - left);
	height = min(height, img->height - top);

	/* Intersect with the bounding box on-screen */
	if((diff = dwindow.left - x) > 0)
	{
		width -= diff;
		left += diff;
		x += diff;
	}
	if((diff = dwindow.top - y) > 0)
	{
		height -= diff;
		top += diff;
		y += diff;
	}
	width = min(width, dwindow.right - x);
	height = min(height, dwindow.bottom - y);

	r->visual_x = x;
	r->y = y;
	r->left = left;
	r->top = top;
	r->width = width;
	r->height = height;

	/* Return non-zero if the result is empty */
	return (width <= 0 || height <= 0);
}

#endif /* GINT_RENDER_MONO */
