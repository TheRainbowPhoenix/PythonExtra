#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <errno.h>
#include <sys/stat.h>
#include "util.h"

int fugue_unlink(char const *path)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

	int type, size, rc;
	rc = BFile_Ext_Stat(fcpath, &type, &size);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		rc = -1;
		goto end;
	}
	if(bfile_type_to_mode_t(type) != S_IFREG) {
		errno = ENOTDIR;
		rc = -1;
		goto end;
	}

	rc = BFile_Remove(fcpath);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		rc = -1;
	}
	else rc = 0;

end:
	free(fcpath);
	return rc;
}
