#include <dirent.h>
#include <unistd.h>

struct dirent *readdir(DIR *dp)
{
	struct dirent *ent = NULL;
	read(dp->fd, &ent, sizeof ent);
	return ent;
}
