#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dsubimage_p4_effect(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, ...)
{
	va_list args;
	va_start(args, eff);

	if(eff & IMAGE_CLEARBG) {
		int bg = va_arg(args, int);
		dsubimage_p4_clearbg(x, y, img, left, top, w, h, eff, bg);
	}
	else if(eff & IMAGE_SWAPCOLOR) {
		int from = va_arg(args, int);
		int to = va_arg(args, int);
		dsubimage_p4_swapcolor(x, y, img, left, top, w, h, eff, from,
			to);
	}
	else if(eff & IMAGE_ADDBG) {
		int bg = va_arg(args, int);
		dsubimage_p4_addbg(x, y, img, left, top, w, h, eff, bg);
	}
	else if(eff & IMAGE_DYE) {
		int dye = va_arg(args, int);
		dsubimage_p4_dye(x, y, img, left, top, w, h, eff, dye);
	}
	else {
		dsubimage_p4(x, y, img, left, top, w, h, eff);
	}

	va_end(args);
}

#endif
