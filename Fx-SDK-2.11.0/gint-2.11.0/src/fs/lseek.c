#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

off_t lseek(int fd, off_t offset, int whence)
{
	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		errno = EINVAL;
		return (off_t)-1;
	}

	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	if(d->type->lseek)
		return d->type->lseek(d->data, offset, whence);

	/* No seek function: cannot seek */
	return 0;
}
