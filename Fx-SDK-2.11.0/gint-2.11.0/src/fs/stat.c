#include <sys/stat.h>
#include "fugue/fugue.h"

int stat(char const * restrict path, struct stat * restrict statbuf)
{
	/* Standard stat() is the Fugue filesystem only */
	return fugue_stat(path, statbuf);
}
