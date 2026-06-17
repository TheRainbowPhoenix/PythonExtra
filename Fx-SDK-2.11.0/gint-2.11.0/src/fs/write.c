#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

ssize_t write(int fd, const void *buf, size_t size)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	if(d->type->write)
		return d->type->write(d->data, buf, size);

	/* No write function: discard the contents but show no error */
	return size;
}
