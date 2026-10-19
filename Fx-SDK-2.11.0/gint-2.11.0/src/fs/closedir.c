#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>

int closedir(DIR *dp)
{
	int rc = close(dp->fd);
	free(dp);
	return rc;
}
