#include <gint/display.h>

/* Based on <http://members.chello.at/~easyfilter/bresenham.html> */
void dcircle(int xm, int ym, int r, int fill_color, int border_color)
{
    if(r < 0 || (fill_color == C_NONE && border_color == C_NONE)) return;

    /* Circle is completely outside the rendering window */
    if(xm-r >= dwindow.right || xm+r < dwindow.left) return;
    if(ym-r >= dwindow.bottom || ym+r < dwindow.top) return;

    int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */

    /* The iteration is slightly changed from the original. We swap x/y in
       quadrants II. and IV. so that we get horizontal linesi nstead of a
       single arc rotated 4 times. However this means we have to go until
       x <= 0 instead of x < 0. */
    do {
        if(fill_color != C_NONE)
            dline(xm-x, ym+y, xm+x, ym+y, fill_color);
        dpixel(xm-x, ym+y, border_color); /*   I. Quadrant */
        dpixel(xm+x, ym+y, border_color); /*  II. Quadrant */

        if(fill_color != C_NONE)
            dline(xm-x, ym-y, xm+x, ym-y, fill_color);
        dpixel(xm+x, ym-y, border_color); /* III. Quadrant */
        dpixel(xm-x, ym-y, border_color); /*  IV. Quadrant */

        r = err;
        if (r <= y) /* e_xy+e_y < 0 */
            err += ++y*2+1;
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
            err += ++x*2+1;
    }
    while (x <= 0);
}
