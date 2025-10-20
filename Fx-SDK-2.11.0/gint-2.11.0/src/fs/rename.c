#include <stdio.h>
#include "fugue/fugue.h"

int rename(char const *oldpath, char const *newpath)
{
	/* Standard rename() is the Fugue filesystem only */
	return fugue_rename(oldpath, newpath);
}
