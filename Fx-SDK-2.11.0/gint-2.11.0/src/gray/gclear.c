#include <gint/gray.h>
#include <gint/config.h>

#if GINT_RENDER_MONO

/* gclear(): Fill the screen with a single color */
void gclear(color_t color)
{
	uint32_t *light, *dark;
	dgray_getvram(&light, &dark);

	uint32_t l = -(color == C_LIGHT || color == C_BLACK);
	uint32_t d = -(color == C_DARK  || color == C_BLACK);

	for(int i = 0; i < 32; i++)
	{
		light[0] = l;
		light[1] = l;
		light[2] = l;
		light[3] = l;

		light[4] = l;
		light[5] = l;
		light[6] = l;
		light[7] = l;

		dark[0] = d;
		dark[1] = d;
		dark[2] = d;
		dark[3] = d;

		dark[4] = d;
		dark[5] = d;
		dark[6] = d;
		dark[7] = d;

		light += 8;
		dark += 8;
	}
}

#endif /* GINT_RENDER_MONO */
