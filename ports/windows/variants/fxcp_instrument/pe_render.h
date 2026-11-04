#ifndef PE_RENDER_H
#define PE_RENDER_H
#include "pe_host.h"
#include <stdlib.h>
#include <stdbool.h>
#define C_NONE (-1)
#define C_INVERT (-2)
#define DWIDTH PE_W
#define DHEIGHT PE_H
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define swap(a,b) do {int t=(a);(a)=(b);(b)=t;} while(0)
#define sgn(x) (((x)>0)-((x)<0))
#define DMODE_OVERRIDE(...)
struct dwindow {int left,top,right,bottom;};
extern struct dwindow dwindow;
void dpixel(int x,int y,int color);
int dgetpixel(int x,int y);
void drect(int x1,int y1,int x2,int y2,int color);
void gint_dhline(int x1,int x2,int y,int color);
void gint_dvline(int y1,int y2,int x,int color);
void dline(int x1,int y1,int x2,int y2,int color);
void drect_border(int x1,int y1,int x2,int y2,int fill,int width,int border);
void dcircle(int x,int y,int r,int fill,int border);
void dellipse(int x1,int y1,int x2,int y2,int fill,int border);
void dpoly(const int *x,const int *y,int n,int fill,int border);
#endif
