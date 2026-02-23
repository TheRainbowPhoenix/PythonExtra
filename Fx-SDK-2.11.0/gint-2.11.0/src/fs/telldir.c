#include <dirent.h>
#include <unistd.h>

long telldir(DIR *dp)
{
	return lseek(dp->fd, 0, SEEK_CUR);
}
