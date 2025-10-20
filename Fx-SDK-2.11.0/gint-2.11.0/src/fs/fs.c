#include <gint/fs.h>
#include <gint/defs/attributes.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

/* File descriptor table */
static fs_descriptor_t *fdtable;

fs_descriptor_t const *fs_get_descriptor(int fd)
{
	if(!fdtable || (unsigned)fd >= FS_FD_MAX)
		return NULL;
	if(fdtable[fd].type == NULL)
		return NULL;

	return &fdtable[fd];
}

int fs_create_descriptor(fs_descriptor_t const *data)
{
	if(!fdtable || data->type == NULL)
		return -1;

	/* Leave 0/1/2 for stdin, stdout and stderr */
	for(int i = 3; i < FS_FD_MAX; i++) {
		if(fdtable[i].type == NULL) {
			fdtable[i] = *data;
			return i;
		}
	}

	return -1;
}

void fs_free_descriptor(int fd)
{
	if(!fdtable || (unsigned)fd >= FS_FD_MAX)
		return;

	fdtable[fd].type = NULL;
	fdtable[fd].data = NULL;
}

int open_generic(fs_descriptor_type_t const *type, void *data, int fd)
{
	if(!fdtable) {
		errno = ENOMEM;
		return -1;
	}
	if(!type) {
		errno = EINVAL;
		return -1;
	}
	fs_descriptor_t d = {
		.type = type,
		.data = data
	};

	/* Re-use file descriptor mode */
	if(fd >= 0) {
		if(fd >= FS_FD_MAX) {
			errno = EBADF;
			return -1;
		}
		if(fdtable[fd].type) {
			errno = ENFILE;
			return -1;
		}

		fdtable[fd] = d;
		return fd;
	}
	/* Normal allocation mode */
	else {
		return fs_create_descriptor(&d);
	}
}

/* Standard streams */

static fs_descriptor_type_t const devnull = {
	.read   = NULL,
	.write  = NULL,
	.lseek  = NULL,
	.close  = NULL,
};

GCONSTRUCTOR static void init_fs(void)
{
	fdtable = calloc(FS_FD_MAX, sizeof *fdtable);
	if(!fdtable)
		return;

	fdtable[STDIN_FILENO].type = &devnull;
	fdtable[STDIN_FILENO].data = NULL;

	fdtable[STDOUT_FILENO].type = &devnull;
	fdtable[STDOUT_FILENO].data = NULL;

	fdtable[STDERR_FILENO].type = &devnull;
	fdtable[STDERR_FILENO].data = NULL;
}
