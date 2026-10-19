//---
//	gint:keycodes - Matrix code for all keyboard keys
//---

#ifndef GINT_KEYCODES
#define GINT_KEYCODES

#ifdef __cplusplus
extern "C" {
#endif

/* Raw matrix codes */
enum {
	KEY_F1		= 0x91,
	KEY_F2		= 0x92,
	KEY_F3		= 0x93,
	KEY_F4		= 0x94,
	KEY_F5		= 0x95,
	KEY_F6		= 0x96,

	KEY_SHIFT	= 0x81,
	KEY_OPTN	= 0x82,
	KEY_VARS	= 0x83,
	KEY_MENU	= 0x84,
	KEY_LEFT	= 0x85,
	KEY_UP		= 0x86,

	KEY_ALPHA	= 0x71,
	KEY_SQUARE	= 0x72,
	KEY_POWER	= 0x73,
	KEY_EXIT	= 0x74,
	KEY_DOWN	= 0x75,
	KEY_RIGHT	= 0x76,

	KEY_XOT		= 0x61,
	KEY_LOG		= 0x62,
	KEY_LN		= 0x63,
	KEY_SIN		= 0x64,
	KEY_COS		= 0x65,
	KEY_TAN		= 0x66,

	KEY_FRAC	= 0x51,
	KEY_FD		= 0x52,
	KEY_LEFTP	= 0x53,
	KEY_RIGHTP	= 0x54,
	KEY_COMMA	= 0x55,
	KEY_ARROW	= 0x56,

	KEY_7		= 0x41,
	KEY_8		= 0x42,
	KEY_9		= 0x43,
	KEY_DEL		= 0x44,
	/* AC/ON has keycode 0x07 instead of 0x45 */

	KEY_4		= 0x31,
	KEY_5		= 0x32,
	KEY_6		= 0x33,
	KEY_MUL		= 0x34,
	KEY_DIV		= 0x35,

	KEY_1		= 0x21,
	KEY_2		= 0x22,
	KEY_3		= 0x23,
	KEY_ADD		= 0x24,
	KEY_SUB		= 0x25,

	KEY_0		= 0x11,
	KEY_DOT		= 0x12,
	KEY_EXP		= 0x13,
	KEY_NEG		= 0x14,
	KEY_EXE		= 0x15,

	/* Why is AC/ON not 0x45? Because it must be on a row/column of its
	   own. It's used to power up the calculator; if it were in the middle
	   of the matrix one could use a ghosting effect to boot the calc. */
	KEY_ACON	= 0x07,

	/* Virtual key codes */
	KEY_HELP	= 0x20, /* fx-9860G Slim: 0x75 */
	KEY_LIGHT	= 0x10, /* fx-9860G Slim: 0x76 */

	/* Key codes for the CP-400 */
	KEY_KBD		= 0xa1,
	KEY_X		= 0xa2,
	KEY_Y		= 0xa3,
	KEY_Z		= 0xa4,
	KEY_EQUALS	= 0xa5,
	KEY_CLEAR       = KEY_EXIT,

	/* Key aliases (handle with care =D) */
	KEY_X2		= KEY_SQUARE,
	KEY_CARET	= KEY_POWER,
	KEY_SWITCH	= KEY_FD,
	KEY_LEFTPAR	= KEY_LEFTP,
	KEY_RIGHTPAR	= KEY_RIGHTP,
	KEY_STORE	= KEY_ARROW,
	KEY_TIMES	= KEY_MUL,
	KEY_PLUS	= KEY_ADD,
	KEY_MINUS	= KEY_SUB,
};

#ifdef __cplusplus
}
#endif

#endif /* GINT_KEYCODES */
