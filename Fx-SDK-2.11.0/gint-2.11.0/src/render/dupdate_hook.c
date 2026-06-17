#include <gint/display.h>

/* Hook to be called after each dupdate(). */
static gint_call_t hook = GINT_CALL_NULL;

/* dupdate_set_hook(): Define a function to be called after each dupdate() */
void dupdate_set_hook(gint_call_t function)
{
	hook = function;
}

/* dupdate_get_hook(): Get a copy of the dupdate() hook */
gint_call_t dupdate_get_hook(void)
{
	return hook;
}
