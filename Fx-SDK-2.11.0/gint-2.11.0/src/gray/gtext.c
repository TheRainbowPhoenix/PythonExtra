#include <gint/gray.h>
#include "../render/render.h"
#include "../render-fx/render-fx.h"
#include <gint/config.h>

#if GINT_RENDER_MONO

/* gtext_opt(): Display a string of text */
void gtext_opt(int x, int y, int fg, int bg, int halign, int valign,
	char const *str, int size)
{
	uint32_t *light, *dark;
	dgray_getvram(&light, &dark);

	if(halign != DTEXT_LEFT || valign != DTEXT_TOP)
	{
		int w, h;
		dsize(str, topti_font, &w, &h);

		if(halign == DTEXT_RIGHT)  x -= w;
		if(halign == DTEXT_CENTER) x -= ((w+1) >> 1);
		if(valign == DTEXT_BOTTOM) y -= h;
		if(valign == DTEXT_MIDDLE) y -= ((h+1) >> 1);
	}

	topti_render(x, y, str, topti_font, topti_asm_text[fg],
		topti_asm_text[bg], light, dark, size);
}

#endif /* GINT_RENDER_MONO */
