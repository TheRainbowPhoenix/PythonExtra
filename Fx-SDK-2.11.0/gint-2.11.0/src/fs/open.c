#include <fcntl.h>
#include <stdarg.h>
#include "fugue/fugue.h"

int open(char const *path, int flags, ...)
{
	va_list args;
	va_start(args, flags);
	mode_t mode = va_arg(args, int);
	va_end(args);

	/* Standard open() is the Fugue filesystem only */
	return fugue_open(path, flags, mode);
}
