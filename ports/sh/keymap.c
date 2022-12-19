#include "keymap.h"
#include <gint/keyboard.h>

static int key_id(int keycode)
{
	uint col = (keycode & 0x0f) - 1;
	uint row = 9 - ((keycode & 0xf0) >> 4);

	if(col > 5 || row > 8) return -1;
	return 6 * row + col;
}

static uint8_t map_flat[30] = {
	 0,    0,   '(',  ')',  ',',  '=',
	'7',  '8',  '9',   0,    0,    0,
	'4',  '5',  '6',  '*',  '/',   0,
	'1',  '2',  '3',  '+',  '-',   0,
	'0',  '.',  'e',  '_',   0,    0,
};
static uint8_t map_alpha[36] = {
	'a',  'b',  'c',  'd',  'e',  'f',
	'g',  'h',  'i',  'j',  'k',  'l',
	'm',  'n',  'o',   0,    0,    0,
	'p',  'q',  'r',  's',  't',   0,
	'u',  'v',  'w',  'x',  'y',   0,
	'z',  ' ',  '"',  ':',   0,    0,
};

uint32_t keymap_translate(int key, bool shift, bool alpha)
{
	int id = key_id(key);
	if(id < 0) return 0;

	if(!shift && !alpha) {
		/* The first 4 rows have no useful characters */
		return (id < 24) ? 0 : map_flat[id - 24];
	}
	if(shift && !alpha) {
		if(key == KEY_MUL) return '{';
		if(key == KEY_DIV) return '}';
		if(key == KEY_ADD) return '[';
		if(key == KEY_SUB) return ']';
		if(key == KEY_DOT) return '=';
		if(key == KEY_0) return ':';
//		if(key == KEY_EXP) return 0x3c0; // 'π'
	}
	if(!shift && alpha) {
		/* The first 3 rows have no useful characters */
		return (id < 18) ? 0 : map_alpha[id - 18];
	}
	if(shift && alpha) {
		int c = keymap_translate(key, false, true);
		return (c >= 'a' && c <= 'z') ? (c & ~0x20) : c;
	}

	return 0;
}
