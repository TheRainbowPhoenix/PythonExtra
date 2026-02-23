#include <unistd.h>
#include "fugue/fugue.h"

int mkdir(char const *path, mode_t mode)
{
	/* Standard mkdir() is the Fugue filesystem only */
	return fugue_mkdir(path, mode);
}
