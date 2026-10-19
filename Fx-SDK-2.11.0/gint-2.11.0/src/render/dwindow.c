#include <gint/display.h>
#include <gint/defs/util.h>

struct dwindow dwindow = {
	.left = 0,
	.top = 0,
	.right = DWIDTH,
	.bottom = DHEIGHT,
};

struct dwindow dwindow_set(struct dwindow m)
{
	m.left = max(m.left, 0);
	m.top = max(m.top, 0);
	m.right = max(m.left, min(m.right, DWIDTH));
	m.bottom = max(m.top, min(m.bottom, DHEIGHT));

	struct dwindow old_mode = dwindow;
	dwindow = m;
	return old_mode;
}
