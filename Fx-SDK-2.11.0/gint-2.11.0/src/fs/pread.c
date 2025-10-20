#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

ssize_t pread(int fd, void *buf, size_t size, off_t offset)
{
	off_t current = lseek(fd, 0, SEEK_CUR);
	if(current == (off_t)-1)
		return (ssize_t)-1;

	ssize_t rc = -1;

	if(lseek(fd, offset, SEEK_SET) == (off_t)-1)
		goto end;

	rc = read(fd, buf, size);
	if(rc < 0)
		goto end;

end:
	/* At the end, always try to restore the current position */
	lseek(fd, current, SEEK_CUR);
	return rc;
}
