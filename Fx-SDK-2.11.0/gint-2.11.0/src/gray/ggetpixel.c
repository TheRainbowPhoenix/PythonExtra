#include <gint/gray.h>
#include <gint/defs/types.h>
#include <gint/config.h>

#if GINT_RENDER_MONO

int ggetpixel(int x, int y)
{
	uint32_t *light, *dark;
	dgray_getvram(&light, &dark);

	int offset = (y << 2) + (x >> 5);
	uint32_t mask = 1 << (~x & 31);

	bool l = (light[offset] & mask) != 0;
	bool d = (dark [offset] & mask) != 0;
	return (d << 1) | l;
}

#endif /* GINT_RENDER_MONO */
