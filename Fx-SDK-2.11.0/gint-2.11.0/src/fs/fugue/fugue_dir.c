#include <gint/fs.h>
#include <gint/hardware.h>
#include <gint/bfile.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "fugue.h"

typedef struct
{
	/* Number of entries */
	int count;
	/* All entries */
	struct dirent **entries;
	/* Current position in directory */
	off_t pos;

} dir_t;

/* There is no well-standardized API for reading from directory descriptors.
   getdents(2) is a thing, but it's mostly Linux-specific and has no glibc
   wrapper, so no userspace application is using it directly. We define our
   directory descriptor interface to mimic opendir(3) for efficiency. */

ssize_t fugue_dir_read(void *data, void *buf, GUNUSED size_t size)
{
	struct dirent **dirent_ptr = buf;
	if(size < sizeof *dirent_ptr)
		return 0;

	dir_t *dp = data;
	if(dp->pos >= dp->count)
		*dirent_ptr = NULL;
	else
		*dirent_ptr = dp->entries[dp->pos++];

	return sizeof *dirent_ptr;
}

ssize_t fugue_dir_write(GUNUSED void *data, GUNUSED const void *buf,
	GUNUSED size_t size)
{
	errno = EISDIR;
	return -1;
}

off_t fugue_dir_lseek(void *data, off_t offset, int whence)
{
	dir_t *dp = data;

	if(whence == SEEK_CUR)
		offset += dp->pos;
	if(whence == SEEK_END)
		offset += dp->count;

	/* dp->count, being at the end of the enumeration, is a valid offset */
	if(offset < 0 || offset >= dp->count + 1) {
		errno = EINVAL;
		return -1;
	}

	dp->pos = offset;
	return dp->pos;
}

int fugue_dir_close(void *data)
{
	dir_t *dp = data;

	if(dp && dp->entries) {
		for(int i = 0; i < dp->count; i++)
			free(dp->entries[i]);
		free(dp->entries);
	}
	free(dp);

	return 0;
}

const fs_descriptor_type_t fugue_dir_descriptor_type = {
	.read   = fugue_dir_read,
	.write  = fugue_dir_write,
	.lseek  = fugue_dir_lseek,
	.close  = fugue_dir_close,
};

void *fugue_dir_explore(char const *path)
{
	struct BFile_FileInfo info;
	char *wildcard=NULL;
	uint16_t *fc_path=NULL, *search=NULL;
	/* We allocate by batches of 8 */
	int sd=-1, rc, allocated=0;

	dir_t *dp = malloc(sizeof *dp);
	if(!dp) goto alloc_failure;

	dp->count = 0;
	dp->entries = NULL;
	dp->pos = 0;

	fc_path = malloc(512 * sizeof *fc_path);
	if(!fc_path) goto alloc_failure;

	wildcard = malloc(strlen(path) + 3);
	if(!wildcard) goto alloc_failure;
	strcpy(wildcard, path);
	strcat(wildcard, "/*");

	search = fs_path_normalize_fc(wildcard);
	if(!search) goto alloc_failure;

	rc = BFile_FindFirst(search, &sd, fc_path, &info);
	if(rc < 0) {
		if(rc != BFile_EntryNotFound)
			errno = bfile_error_to_errno(rc);
		goto error;
	}

	do {
		if(dp->count+1 > allocated) {
			struct dirent **new_entries = realloc(dp->entries,
				(allocated + 8) * sizeof *dp->entries);
			if(!new_entries)
				goto alloc_failure;
			dp->entries = new_entries;
			allocated += 8;
		}

		size_t name_length = fc_len(fc_path);
		size_t s = sizeof(struct dirent) + name_length + 1;
		struct dirent *ent = malloc(s);
		if(!ent) goto alloc_failure;

		ent->d_ino = 0;
		ent->d_type = bfile_type_to_dirent(info.type);
		fc_to_utf8(ent->d_name, fc_path, name_length + 1);
		dp->entries[dp->count++] = ent;

		rc = BFile_FindNext(sd, fc_path, &info);
	}
	while(rc >= 0);
	goto end;

alloc_failure:
	errno = ENOMEM;
error:
	fugue_dir_close(dp);
	dp = NULL;
end:
	free(wildcard);
	free(search);
	free(fc_path);
	if(sd >= 0)
		BFile_FindClose(sd);
	return dp;
}
