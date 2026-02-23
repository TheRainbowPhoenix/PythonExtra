#include <gint/display.h>
#include <gint/defs/util.h>

/* Based on <http://members.chello.at/~easyfilter/bresenham.html> */
void dellipse(int x1, int y1, int x2, int y2, int fill_color, int border_color)
{
	if(fill_color == C_NONE && border_color == C_NONE) return;

	if(x1 > x2) swap(x1, x2);
	if(y1 > y2) swap(y1, y2);

	/* Ellipse is completely outside the rendering window */
	if(x1 >= dwindow.right || x2 < dwindow.left) return;
	if(y1 >= dwindow.bottom || y2 < dwindow.top) return;

	int a = x2-x1, b = y2-y1, b1 = b&1; /* diameter */
	int dx = 4*(1-a)*b*b, dy = 4*(b1+1)*a*a; /* error increment */
	int err = dx + dy + b1 * a * a, e2; /* error of 1.step */

	y1 += (b + 1) / 2;
	y2 = y1-b1; /* starting pixel */
	a = 8 * a * a;
	b1 = 8 * b * b;

	do {
		if(fill_color != C_NONE)
			dline(x1, y1, x2, y1, fill_color);
		dpixel(x2, y1, border_color); /* I. Quadrant */
		dpixel(x1, y1, border_color); /* II. Quadrant */

		if(fill_color != C_NONE)
			dline(x1, y2, x2, y2, fill_color);
		dpixel(x1, y2, border_color); /* III. Quadrant */
		dpixel(x2, y2, border_color); /* IV. Quadrant */

		e2 = 2 * err;
		if (e2 <= dy) {
			y1++;
			y2--;
			err += (dy += a);
		}
		if (e2 >= dx || 2 * err > dy) {
			x1++;
			x2--;
			err += (dx += b1);
		}
	}
	while (x1 <= x2);

	while (y1 - y2 <= b) /* to early stop of flat ellipses a=1 */
	{
		dpixel(x1 - 1, y1, border_color); /* finish tip of ellipse */
		dpixel(x2 + 1, y1++, border_color);
		dpixel(x1 - 1, y2, border_color);
		dpixel(x2 + 1, y2--, border_color);
	}
}
