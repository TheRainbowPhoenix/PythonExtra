#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

int close(int fd)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	int rc = 0;
	if(d->type->close)
		rc = d->type->close(d->data);

	fs_free_descriptor(fd);
	return rc;
}
