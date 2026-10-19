#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include "util.h"
#include "fugue.h"

int fugue_rmdir(char const *path)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	/* Check if the folder is empty */
	DIR *dp = opendir(path);
	if(!dp) return -1;

	bool empty = true;
	struct dirent *ent;

	while((ent = readdir(dp))) {
		if(strcmp(ent->d_name, ".") != 0 &&
		   strcmp(ent->d_name, "..") != 0) {
			empty = false;
			break;
		}
	}

	closedir(dp);

	if(!empty) {
		errno = ENOTEMPTY;
		return -1;
	}

	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

	int rc = BFile_Remove(fcpath);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		rc = -1;
	}
	else rc = 0;

	free(fcpath);
	return rc;
}
