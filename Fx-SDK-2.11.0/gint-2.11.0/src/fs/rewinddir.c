#include <dirent.h>
#include <unistd.h>

void rewinddir(DIR *dp)
{
	lseek(dp->fd, 0, SEEK_SET);
}
