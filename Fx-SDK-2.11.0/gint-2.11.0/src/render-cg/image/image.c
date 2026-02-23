#include <gint/image.h>
#include <gint/display.h>
#include <gint/defs/util.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

bool gint_image_clip_input(image_t const *img, struct gint_image_box *b,
	struct dwindow const *window)
{
	/* Adjust the bounding box of the input image */
	if(b->left < 0) b->w += b->left, b->x -= b->left, b->left = 0;
	if(b->top  < 0) b->h += b->top,  b->y -= b->top,  b->top  = 0;
	if(b->left + b->w > img->width)  b->w = img->width  - b->left;
	if(b->top  + b->h > img->height) b->h = img->height - b->top;

	/* Check whether the box intersects the screen */
	if(b->w <= 0 || b->h <= 0)
		return false;
	if(b->x + b->w <= window->left || b->x >= window->right)
		return false;
	if(b->y + b->h <= window->top || b->y >= window->bottom)
		return false;

	return true;
}

void gint_image_clip_output(struct gint_image_box *b,
	struct dwindow const *window)
{
	/* Intersect with the bounding box on-screen */

	if(b->y < window->top) {
		int d = window->top - b->y; /* > 0 */
		b->top += d;
		b->h -= d;
		b->y += d;
	}
	b->h = min(b->h, window->bottom - b->y);

	if(b->x < window->left) {
		int d = window->left - b->x; /* > 0 */
		b->left += d;
		b->w -= d;
		b->x += d;
	}
	b->w = min(b->w, window->right - b->x);
}

bool gint_image_mkcmd(struct gint_image_box *box, image_t const *img,
	int effects, bool left_edge, bool right_edge,
	struct gint_image_cmd *cmd, struct dwindow const *window)
{
	/* Convert the old DIMAGE_NOCLIP flag */
	if(effects & DIMAGE_NOCLIP)
		effects |= IMAGE_NOCLIP;

	if(!(effects & IMAGE_NOCLIP_INPUT)) {
		if(!gint_image_clip_input(img, box, window))
			return false;
	}
	if(!(effects & IMAGE_NOCLIP_OUTPUT))
		gint_image_clip_output(box, window);

	cmd->effect = (effects & (IMAGE_VFLIP | IMAGE_HFLIP)) >> 8;
	cmd->columns = box->w;
	cmd->input_stride = img->stride;
	cmd->x = box->x;
	cmd->edge_1 = -1;
	cmd->edge_2 = -1;

	int f = img->format;
	int input_row = (effects & IMAGE_VFLIP) ? box->top+box->h-1 : box->top;

	if(IMAGE_IS_RGB16(f)) {
		cmd->input_stride += (cmd->input_stride & 1);
		cmd->input = (void *)img->data +
			input_row * img->stride + (box->left * 2);
	}
	else if(IMAGE_IS_P8(f)) {
		cmd->input = (void *)img->data +
			(input_row * img->stride) + box->left;
		cmd->palette = (void *)img->palette + 256;
	}
	else {
		cmd->input = (void *)img->data +
			input_row * img->stride + (box->left >> 1);
		cmd->palette = (void *)img->palette;
		/* By default, use edge_1 to indicate (box->left & 1), so that
		   functions that don't use edge_1 can still work properly */
		if(!left_edge)
			cmd->edge_1 = (box->left & 1);
	}

	if(left_edge && (box->left & 1)) {
		if(effects & IMAGE_HFLIP) {
			cmd->edge_1 = cmd->columns;
		}
		else {
			cmd->x--;
			cmd->edge_1 = 0;
		}
		cmd->columns++;
	}
	if(right_edge && (cmd->columns & 1)) {
		if(effects & IMAGE_HFLIP) {
			cmd->x--;
			cmd->edge_1++;
			cmd->edge_2 = 0;
		}
		else {
			cmd->edge_2 = cmd->columns;
		}
		cmd->columns++;
	}

	/* Settings for further updates */
	cmd->height = box->h;

	/* This is the default for gint, but Azur overwrites it */
	cmd->lines = box->h;
	cmd->output = (void *)gint_vram + (DWIDTH * box->y + cmd->x) * 2;
	return true;
}

#endif
