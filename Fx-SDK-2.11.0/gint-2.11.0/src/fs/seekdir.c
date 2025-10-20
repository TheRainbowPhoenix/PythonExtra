#include <dirent.h>
#include <unistd.h>

void seekdir(DIR *dp, long loc)
{
	lseek(dp->fd, loc, SEEK_SET);
}
