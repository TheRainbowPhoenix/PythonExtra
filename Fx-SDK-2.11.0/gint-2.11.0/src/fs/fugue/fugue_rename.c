#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <errno.h>
#include <sys/stat.h>
#include "util.h"

int fugue_rename(char const *oldpath, char const *newpath)
{
	ENOTSUP_IF_NOT_FUGUE(-1);
	int rc = -1;

	uint16_t *fcpath_1 = NULL;
	uint16_t *fcpath_2 = NULL;

	fcpath_1 = fs_path_normalize_fc(oldpath);
	if(!fcpath_1) {
		errno = ENOMEM;
		goto end;
	}
	fcpath_2 = fs_path_normalize_fc(newpath);
	if(!fcpath_2) {
		errno = ENOMEM;
		goto end;
	}

	rc = BFile_Rename(fcpath_1, fcpath_2);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		rc = -1;
	}
	else rc = 0;

end:
	free(fcpath_1);
	free(fcpath_2);
	return rc;
}
