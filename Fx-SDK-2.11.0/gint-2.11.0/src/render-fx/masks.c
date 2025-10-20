#include <gint/defs/util.h>
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

void masks(int x1, int x2, uint32_t *masks)
{
	x1 = max(x1, dwindow.left);
	x2 = min(x2, dwindow.right - 1);

	/* Indexes of the first and last non-empty longs */
	size_t l1 = x1 >> 5;
	size_t l2 = x2 >> 5;
	size_t i = 0;

	while(i < l1)	masks[i++] = 0x00000000;
	while(i <= l2)	masks[i++] = 0xffffffff;
	while(i < 4)	masks[i++] = 0x00000000;

	/* Remove the index information in x1 and x2; keep only the offsets */
	x1 &= 31;
	x2 = ~x2 & 31;

	masks[l1] &= (0xffffffffu >> x1);
	masks[l2] &= (0xffffffffu << x2);
}

#endif /* GINT_RENDER_MONO */
