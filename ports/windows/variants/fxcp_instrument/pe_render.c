#include "pe_render.h"
struct dwindow dwindow={0,0,PE_W,PE_H};
void dpixel(int x,int y,int color) {
    if(x<dwindow.left||x>=dwindow.right||y<dwindow.top||y>=dwindow.bottom||color==C_NONE) {return;}
    uint16_t *p=pe_vram+y*PE_W+x;
    *p=color==C_INVERT?~*p:(uint16_t)color;
    pe_pixel_count(1);
}
int dgetpixel(int x,int y) {
    if(x<0||x>=PE_W||y<0||y>=PE_H) {return C_NONE;}
    return pe_vram[y*PE_W+x];
}
void drect(int x1,int y1,int x2,int y2,int color) {
    pe_init();
    if(x1>x2) {swap(x1,x2);} if(y1>y2) {swap(y1,y2);}
    x1=max(x1,dwindow.left);x2=min(x2,dwindow.right-1);
    y1=max(y1,dwindow.top);y2=min(y2,dwindow.bottom-1);
    if(x1>x2||y1>y2||color==C_NONE) {return;}
    for(int y=y1;y<=y2;y++) {
        uint16_t *row=pe_vram+y*PE_W;
        for(int x=x1;x<=x2;x++) {row[x]=color==C_INVERT?~row[x]:(uint16_t)color;}
    }
    pe_pixel_count((x2-x1+1)*(y2-y1+1));
}
void gint_dhline(int x1,int x2,int y,int c) {drect(x1,y,x2,y,c);}
void gint_dvline(int y1,int y2,int x,int c) {drect(x,y1,x,y2,c);}
