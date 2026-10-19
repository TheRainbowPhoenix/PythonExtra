#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <errno.h>
#include "util.h"

int fugue_mkdir(char const *path, GUNUSED mode_t mode)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

	int rc = BFile_Create(fcpath, BFile_Folder, NULL);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		free(fcpath);
		return -1;
	}

	free(fcpath);
	return 0;
}
