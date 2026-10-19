#include <gint/bfile.h>
#include "util.h"

int BFile_Ext_Stat(uint16_t const *path, int *type, int *size)
{
	int search_handle, rc;
	uint16_t found_file[256];
	struct BFile_FileInfo fileinfo;

	rc = BFile_FindFirst(path, &search_handle, found_file, &fileinfo);
	if(rc < 0) {
		if(type) *type = -1;
		if(size) *size = -1;
		rc = -1;
	}
	else {
		if(type) *type = fileinfo.type;
		if(size) *size = fileinfo.file_size;
		rc = 0;
	}

	BFile_FindClose(search_handle);
	return rc;
}
