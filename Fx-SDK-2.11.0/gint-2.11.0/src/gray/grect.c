#include <gint/defs/util.h>
#include <gint/gray.h>
#include "../render-fx/render-fx.h"
#include <gint/config.h>

#if GINT_RENDER_MONO

/* grect(): Fill a rectangle on the screen */
void grect(int x1, int y1, int x2, int y2, color_t color)
{
	/* Use masks to get the work done fast! */
	uint32_t m[4];
	masks(x1, x2, m);

	uint32_t *light, *dark;
	dgray_getvram(&light, &dark);

	light += (y1 << 2);
	dark  += (y1 << 2);
	int h = y2 - y1 + 1;

	if(color == C_WHITE) while(h--)
	{
		light[0] &= ~m[0];
		light[1] &= ~m[1];
		light[2] &= ~m[2];
		light[3] &= ~m[3];

		dark[0] &= ~m[0];
		dark[1] &= ~m[1];
		dark[2] &= ~m[2];
		dark[3] &= ~m[3];

		light += 4;
		dark += 4;
	}
	else if(color == C_LIGHT) while(h--)
	{
		light[0] |= m[0];
		light[1] |= m[1];
		light[2] |= m[2];
		light[3] |= m[3];

		dark[0] &= ~m[0];
		dark[1] &= ~m[1];
		dark[2] &= ~m[2];
		dark[3] &= ~m[3];

		light += 4;
		dark += 4;
	}
	else if(color == C_DARK) while(h--)
	{
		light[0] &= ~m[0];
		light[1] &= ~m[1];
		light[2] &= ~m[2];
		light[3] &= ~m[3];

		dark[0] |= m[0];
		dark[1] |= m[1];
		dark[2] |= m[2];
		dark[3] |= m[3];

		light += 4;
		dark += 4;
	}
	else if(color == C_BLACK) while(h--)
	{
		light[0] |= m[0];
		light[1] |= m[1];
		light[2] |= m[2];
		light[3] |= m[3];

		dark[0] |= m[0];
		dark[1] |= m[1];
		dark[2] |= m[2];
		dark[3] |= m[3];

		light += 4;
		dark += 4;
	}
	else if(color == C_INVERT) while(h--)
	{
		light[0] ^= m[0];
		light[1] ^= m[1];
		light[2] ^= m[2];
		light[3] ^= m[3];

		dark[0] ^= m[0];
		dark[1] ^= m[1];
		dark[2] ^= m[2];
		dark[3] ^= m[3];

		light += 4;
		dark += 4;
	}
	else if(color == C_LIGHTEN) for(int i = 0; i < (h << 2); i++)
	{
		int j = i & 3;
		uint32_t tmp = *dark, x = m[j];

		*dark &= *light | ~x;
		*light = (*light ^ x) & (tmp | ~x);

		light++, dark++;
	}
	else if(color == C_DARKEN) for(int i = 0; i < (h << 2); i++)
	{
		int j = i & 3;
		uint32_t tmp = *dark, x = m[j];

		*dark |= *light & x;
		*light = (*light ^ x) | (tmp & x);

		light++, dark++;
	}
}

#endif /* GINT_RENDER_MONO */
