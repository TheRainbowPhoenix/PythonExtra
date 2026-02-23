#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

DIR *opendir(const char *name)
{
	DIR *dp = malloc(sizeof *dp);
	if(!dp) {
		errno = ENOMEM;
		return NULL;
	}

	int fd = open(name, O_DIRECTORY | O_RDONLY);
	if(fd < 0) {
		free(dp);
		return NULL;
	}

	dp->fd = fd;
	return dp;
}
