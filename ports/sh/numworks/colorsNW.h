// Data can be found here
// https://github.com/numworks/epsilon/blob/master/escher/include/escher/palette.h
// and here
// https://github.com/numworks/epsilon/blob/master/python/port/port.cpp#L221

#define NW_RGB(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 6) | (b >> 3))

#define NW_BLUE NW_RGB(0x50, 0x75, 0xF2)
#define NW_RED NW_RGB(0xFF, 0x00, 0x0C)
#define NW_GREEN NW_RGB(0x50, 0xC1, 0x02)
#define NW_WHITE NW_RGB(0xF7, 0xF9, 0xFA)
#define NW_BLACK NW_RGB(0x00, 0x00, 0x00)

#define NW_YELLOW NW_RGB(0xFF, 0xCC, 0x7B)
#define NW_PURPLE NW_RGB(0x6E, 0x2D, 0x79)
#define NW_BROWN NW_RGB(0x8D, 0x73, 0x50)
#define NW_CYAN NW_RGB(0x00, 0xFF, 0xFF)
#define NW_ORANGE NW_RGB(0xFE, 0x87, 0x1F)
#define NW_PINK NW_RGB(0xFF, 0xAB, 0xB6)
#define NW_MAGENTA NW_RGB(0xFF, 0x05, 0x88)
#define NW_GRAY NW_RGB(0xA7, 0xA7, 0xA7)
