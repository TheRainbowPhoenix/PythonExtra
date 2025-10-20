#include <unistd.h>
#include "fugue/fugue.h"

int rmdir(char const *path)
{
	/* Standard rmdir() is the Fugue filesystem only */
	return fugue_rmdir(path);
}
