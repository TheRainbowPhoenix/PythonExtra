//---
//	gint:keysc:getkey - High-level keyboard monitoring function
//---

#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <gint/display.h>
#include <gint/gint.h>
#include <gint/defs/types.h>

#if GINT_HW_FX
#include <gint/drivers/t6k11.h>
#endif

/* Feature function */
static getkey_feature_t feature_function = NULL;

/* getkey(): Wait for a key press */
key_event_t getkey(void)
{
	return getkey_opt(GETKEY_DEFAULT, NULL);
}

/* getkey_opt(): Enhanced getkey() */
key_event_t getkey_opt(int opt, volatile int *timeout)
{
	keydev_t *d = keydev_std();
	keydev_transform_t tr = keydev_transform(d);
	key_event_t e;

	int o = KEYDEV_TR_REPEATS +
		KEYDEV_TR_DELETE_MODIFIERS +
		KEYDEV_TR_DELETE_RELEASES +
		(opt & (GETKEY_MOD_SHIFT + GETKEY_MOD_ALPHA));
	keydev_set_transform(d, (keydev_transform_t){ o, tr.repeater });

	while(1)
	{
		e = keydev_read(d, true, timeout);
		if(e.type == KEYEV_NONE && timeout && *timeout) break;

		/* Skip repeat events that are not enabled by options */
		if(e.type == KEYEV_HOLD && !(opt & GETKEY_REP_ALL))
		{
			if(!(opt & GETKEY_REP_ARROWS))
				continue;
			if(e.key != KEY_LEFT && e.key != KEY_RIGHT &&
			   e.key != KEY_UP && e.key != KEY_DOWN)
				continue;
		}

		#if GINT_HW_FX
		/* Backlight toggle */
		if((opt & GETKEY_BACKLIGHT) && e.type == KEYEV_DOWN &&
			((e.key == KEY_LIGHT) ||
				(e.key == KEY_OPTN && e.shift && !e.alpha)))
		{
			t6k11_backlight(-1);
			continue;
		}
		#endif

		/* Return-to-menu */
		if((opt & GETKEY_MENU) && e.type == KEYEV_DOWN &&
			e.key == KEY_MENU && !e.shift && !e.alpha)
		{
			gint_osmenu();
			if(opt & GETKEY_MENU_DUPDATE)
				dupdate();

			if(!(opt & GETKEY_MENU_EVENT))
				continue;
			e.type = KEYEV_OSMENU;
		}

		/* Poweroff */
		if((opt & GETKEY_POWEROFF) && e.type == KEYEV_DOWN &&
			e.key == KEY_ACON && e.shift && !e.alpha)
		{
			gint_poweroff(true);
			if(opt & GETKEY_MENU_DUPDATE)
				dupdate();
			continue;
		}

		if(e.type != KEYEV_NONE || e.type != KEYEV_UP)
		{
			/* Custom global features */
			bool accepted = false;
			if((opt & GETKEY_FEATURES) && feature_function)
				accepted = feature_function(e);
			/* Return if the event has not been accepted yet */
			if(!accepted) break;
		}
	}

	/* Restore previous transform settings */
	keydev_set_transform(d, tr);
	return e;
}

/* getkey_feature_function(): Get the current feature function */
getkey_feature_t getkey_feature_function(void)
{
	return feature_function;
}

/* getkey_set_feature_function(): Set the global feature function */
void getkey_set_feature_function(getkey_feature_t function)
{
	feature_function = function;
}

/* Deprecated repeat functions */

void getkey_repeat(int first, int next)
{
	keydev_set_standard_repeats(keydev_std(), first * 1000, next * 1000);
}

getkey_profile_t getkey_repeat_profile(void)
{
	return keydev_transform(keydev_std()).repeater;
}

/* getkey_set_repeat_profile(): Set the repeat profile function */
void getkey_set_repeat_profile(getkey_profile_t profile)
{
	keydev_transform_t tr = keydev_transform(keydev_std());
	tr.repeater = profile;
	keydev_set_transform(keydev_std(), tr);
}
