#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

ssize_t read(int fd, void *buf, size_t size)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	if(d->type->read)
		return d->type->read(d->data, buf, size);

	/* No read function: we can't read anything */
	return 0;
}
