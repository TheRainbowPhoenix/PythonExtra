#include <gint/defs/util.h>
#include <gint/display.h>
#include <stdlib.h>

static int compare(void const *ptr1, void const *ptr2)
{
	return *(int const *)ptr1 - *(int const *)ptr2;
}

static void dpoly_fill(int const *x, int const *y, int N, int color)
{
	int *nodeX = malloc(N * sizeof *nodeX);
	if(!nodeX)
		return;

	/* Find vertical bounds */
	int ymin = y[0], ymax = y[0];
	for(int i = 1; i < N; i++)
	{
		ymin = min(ymin, y[i]);
		ymax = max(ymax, y[i]);
	}
	ymin = max(ymin, dwindow.top);
	ymax = min(ymax, dwindow.bottom);

	/* For each row, find vertical cuts from the segments and fill in-between
	   the cuts. */
	for(int pixelY = ymin; pixelY <= ymax; pixelY++) {

		/* Build a list of nodes */
		int nodes = 0;
		int j = N - 1;
		for(int i = 0; i < N; i++)
		{
			if((y[i] < pixelY) != (y[j] < pixelY))
				nodeX[nodes++] = x[i]+(pixelY-y[i])*(x[j]-x[i])/(y[j]-y[i]);
			j = i;
		}

		/* Sort the cuts' positions */
		qsort(nodeX, nodes, sizeof nodeX[0], compare);

		/* Fill the pixels between cut pairs */
		for(int i = 0; i < nodes; i += 2)
		{
			if(nodeX[i] >= dwindow.right || nodeX[i+1] < dwindow.left) break;
			nodeX[i] = max(nodeX[i], dwindow.left);
			nodeX[i+1] = min(nodeX[i+1], dwindow.right);
			dline(nodeX[i], pixelY, nodeX[i+1], pixelY, color);
		}
	}

	free(nodeX);
}

static void dpoly_border(int const *x, int const *y, int N, int color)
{
	for(int i = 0; i < N - 1; i++)
		dline(x[i], y[i], x[i+1], y[i+1], color);
	dline(x[N-1], y[N-1], x[0], y[0], color);
}

void dpoly(int const *x, int const *y, int N, int fill_color, int border_color)
{
	if(N > 2 && fill_color != C_NONE)
		dpoly_fill(x, y, N, fill_color);
	if(N >= 2 && border_color != C_NONE)
		dpoly_border(x, y, N, border_color);
}
