#include <gint/gint.h>
#include <gint/display.h>
#include <gint/hardware.h>
#include <gint/keyboard.h>
#include "kernel.h"

#include <string.h>

int   __Timer_Install(int id, void (*handler)(void), int delay);
int   __Timer_Start(int id);
int   __Timer_Stop(int id);
int   __Timer_Deinstall(int id);
int   __PutKeyCode(int row, int column, int keycode);
int   __GetKeyWait(int *col,int *row,int type,int time,int menu,uint16_t *key);
void  __ClearKeyBuffer(void); /* ? */
void  __ConfigureStatusArea(int mode);
void __SetQuitHandler(void (*callback)(void));

#if !GINT_OS_CP
static int __osmenu_id;

static void __osmenu_handler(void)
{
	if(isSlim())
		__PutKeyCode(0x07, 0x0A, 0);
	else
		__PutKeyCode(0x04, 0x09, 0);

	__Timer_Stop(__osmenu_id);
	__Timer_Deinstall(__osmenu_id);
}
#endif

#if GINT_OS_CG
typedef void (os_menu_function_t)(void);

/* This method is possible thanks to reverse-engineering by Dr-Carlos.
   <https://www.cemetech.net/forum/viewtopic.php?t=18944> */
static os_menu_function_t *find_os_menu_function(void)
{
	/* Get syscall table address */
	uint32_t addr = *(uint32_t *)0x8002007c;
	if(addr < 0x80020070 || addr >= 0x81000000 - 0x1e58 * 4)
		return NULL;

	/* Get pointer to %1e58 SwitchToMainMenu() */
	uint16_t const *insns = *(uint16_t const **)(addr + 0x1e58 * 4);
	if(addr < 0x80020070 || addr >= 0x81000000)
		return NULL;

	/* Check up to 150 instructions to find the call to the internal function
	   SaveAndOpenMainMenu(). This call is in a widget of the shape

	     mov.l	GetkeyToMainFunctionReturnFlag, rX
	     mov	#3, rY
	     bsr	SaveAndOpenMainMenu
	     mov.b	rY, @rX
	     bra	<start of widget>
	     nop */
	for(int i = 0; i < 150; i++) {
		/* Match: mov.l @(disp, pc), rX */
		if((insns[i] & 0xf000) != 0xd000)
			continue;
		int rX = (insns[i] >> 8) & 0x0f;

		/* Match: mov #3, rY */
		if((insns[i+1] & 0xf0ff) != 0xe003)
			continue;
		int rY = (insns[i+1] >> 8) & 0x0f;

		/* Match: bsr @(disp, pc) */
		if((insns[i+2] & 0xf000) != 0xb000)
			continue;
		int disp = (insns[i+2] & 0x0fff);

		/* Match: mov.b rX, @rY */
		if((insns[i+3] != 0x2000 + (rX << 8) + (rY << 4)))
			continue;

		/* Match: bra @(_, pc) */
		if((insns[i+4] & 0xf000) != 0xa000)
			continue;

		/* Match: nop */
		if(insns[i+5] != 0x0009)
			continue;

		/* Return the target of the bsr instruction */
		uint32_t fun_addr = (uint32_t)&insns[i+2] + 4 + disp * 2;
		return (os_menu_function_t *)fun_addr;
	}

	return NULL;
}
#endif

void gint_osmenu_native(void)
{
// TODO: OS menu on fx-CP
#if !GINT_OS_CP
	__ClearKeyBuffer();
	gint_copy_vram();

	#if GINT_OS_CG
	/* Unfortunately ineffective (main menu probably reenables it)
	__ConfigureStatusArea(3); */

	/* Try to use the internal function directly if we could figure out its
	   address by dynamically disassembling */
	os_menu_function_t *fun = find_os_menu_function();
	if(fun) {
		fun();

		/* Run an immediate keyboard update, and clear the events so that the
		   key pressed in order to re-enter the add-in is not also processed in
		   the application */
		extern int keysc_tick(void);
		keysc_tick();
		clearevents();

		return;
	}
	#endif

	/* Mysteriously crashes when coming back; might be useful another time
	   instead of GetKeyWait()
	int C=0x04, R=0x09;
	__SpecialMatrixCodeProcessing(&C, &R); */

	__osmenu_id = __Timer_Install(0, __osmenu_handler, 0 /* ms */);
	if(__osmenu_id <= 0) return;
	__Timer_Start(__osmenu_id);

	int column, row;
	unsigned short keycode;
	__GetKeyWait(&column, &row,
		0 /* KEYWAIT_HALTON_TIMEROFF */,
		1 /* Delay in seconds */,
		0 /* Enable return to main menu */,
		&keycode);
#endif
}

/* gint_osmenu() - switch out of gint and call the calculator's main menu */
void gint_osmenu(void)
{
	gint_world_switch(GINT_CALL(gint_osmenu_native));
}

static gint_call_t __gcall;
static bool __do_world_switch;

static void __handler()
{
	if(__do_world_switch){
		gint_call(__gcall);
	}else{
		/* TODO: quit the world switch */
		gint_call(__gcall);
	}
}

static void __sethandler()
{
	(void)__handler;
// TODO: Quit handler on fx-CP
#if !GINT_OS_CP
	__SetQuitHandler((void *)__handler);
#endif
}

void gint_set_quit_handler(gint_call_t gcall, bool do_world_switch)
{
	__gcall = gcall;
	__do_world_switch = do_world_switch;
	gint_world_switch(GINT_CALL(__sethandler));
}
