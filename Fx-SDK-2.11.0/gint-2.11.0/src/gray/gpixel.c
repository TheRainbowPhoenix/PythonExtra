#include <gint/gray.h>
#include <gint/defs/types.h>
#include <gint/config.h>

#if GINT_RENDER_MONO

/* gpixel(): Change a pixel's color */
void gpixel(int x, int y, color_t color)
{
	uint32_t *light, *dark;
	dgray_getvram(&light, &dark);

	int offset = (y << 2) + (x >> 5);
	uint32_t mask = 1 << (~x & 31), tmp;

	switch(color)
	{
	case C_WHITE:
		light[offset] &= ~mask;
		dark [offset] &= ~mask;
		break;

	case C_LIGHT:
		light[offset] |= mask;
		dark [offset] &= ~mask;
		break;

	case C_DARK:
		light[offset] &= ~mask;
		dark [offset] |= mask;
		break;

	case C_BLACK:
		light[offset] |= mask;
		dark [offset] |= mask;
		break;

	case C_NONE:
		break;

	case C_INVERT:
		light[offset] ^= mask;
		dark [offset] ^= mask;
		break;

	case C_LIGHTEN:
		tmp = dark[offset];

		dark[offset] &= light[offset] | ~mask;
		light[offset] = (light[offset] ^ mask) & (tmp | ~mask);
		break;

	case C_DARKEN:
		tmp = dark[offset];

		dark[offset] |= light[offset] & mask;
		light[offset] = (light[offset] ^ mask) | (tmp & mask);
		break;
	}
}

#endif /* GINT_RENDER_MONO */
