#include <unistd.h>
#include "fugue/fugue.h"

int unlink(char const *path)
{
	/* Standard unlink() is the Fugue filesystem only */
	return fugue_unlink(path);
}
