#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <gint/bfile.h>
#include <dirent.h>
#include <sys/stat.h>

int bfile_error_to_errno(int e)
{
	/* TODO: Find BFile code for too many fds and map it to ENFILE. */
	switch(e) {
	case BFile_EntryNotFound:	return ENOENT;
	case BFile_IllegalPath:		return EINVAL;
	case BFile_DeviceFull:		return EDQUOT;
	case BFile_IllegalDevice:	return EINVAL;
	case BFile_AccessDenied:	return EACCES;
	case BFile_PermissionError:	return EACCES;
	case BFile_EntryFull:		return EDQUOT;
	case BFile_AlreadyExists:	return EEXIST;
	case BFile_ReadOnlyFile:	return EACCES;
	case BFile_EnumerateEnd:	return ENOENT;
	case BFile_IllegalSeekPos:	return EINVAL;
	case BFile_NotMountDevice:	return ENOENT;
	case BFile_DeviceNotFound:	return ENOENT;
	default:			return errno;
	}
}

int bfile_type_to_mode_t(int bfile_type)
{
	switch(bfile_type) {
	case BFile_Type_Directory:      return S_IFDIR;
	case BFile_Type_Dot:            return S_IFDIR;
	case BFile_Type_DotDot:         return S_IFDIR;
	case BFile_Type_MainMem:        return S_IFBLK;
	case BFile_Type_File:           return S_IFREG;
	case BFile_Type_Archived:       return S_IFREG;
	default:                        return S_IFREG;
	}
}

int bfile_type_to_dirent(int bfile_type)
{
	switch(bfile_type) {
	case BFile_Type_Directory:      return DT_DIR;
	case BFile_Type_Dot:            return DT_DIR;
	case BFile_Type_DotDot:         return DT_DIR;
	case BFile_Type_MainMem:        return DT_BLK;
	case BFile_Type_File:           return DT_REG;
	case BFile_Type_Archived:       return DT_REG;
	default:                        return DT_UNKNOWN;
	}
}

/* Length of FONTCHARACTER and UTF-8 strings, counting only ASCII characters */
size_t utf8_len(char const *utf8)
{
	size_t len = 0;
	for(size_t i = 0; utf8[i] != 0; i++)
		len += (utf8[i] >= 0 && (uint8_t)utf8[i] <= 0x7f);
	return len;
}
size_t fc_len(uint16_t const *fc)
{
	size_t len = 0;
	for(size_t i = 0; fc[i] != 0 && fc[i] != 0xffff; i++)
		len += (fc[i] <= 0x7f);
	return len;
}

void utf8_to_fc(uint16_t *fc, char const *utf8, size_t fc_len)
{
	size_t j = 0;

	for(size_t i = 0; j < fc_len && utf8[i] != 0; i++) {
		if(utf8[i] == '/')
			fc[j++] = '\\';
		else if(utf8[i] >= 0 && (uint8_t)utf8[i] <= 0x7f)
			fc[j++] = utf8[i];
	}

	if(j < fc_len)
		fc[j++] = 0;
}

void fc_to_utf8(char *utf8, uint16_t const *fc, size_t utf8_len)
{
	size_t j = 0;

	for(size_t i = 0; j < utf8_len && fc[i] != 0 && fc[i] != 0xffff; i++) {
		if(fc[i] == '\\')
			utf8[j++] = '/';
		else if(fc[i] <= 0x7f)
			utf8[j++] = fc[i];
	}

	if(j < utf8_len)
		utf8[j++] = 0;
}

uint16_t *utf8_to_fc_alloc(uint16_t *prefix, char const *utf8,
	uint16_t *suffix)
{
	size_t lenp=0, lens=0;
	if(prefix) {
		while(prefix[lenp] != 0 && prefix[lenp] != 0xffff)
			lenp++;
	}
	if(suffix) {
		while(suffix[lens] != 0 && suffix[lens] != 0xffff)
			lens++;
	}

	size_t len = utf8_len(utf8);
	uint16_t *fc = malloc((lenp+len+lens+1) * sizeof *fc);

	if(fc != NULL) {
		if(prefix)
			memcpy(fc, prefix, lenp * sizeof *prefix);
		utf8_to_fc(fc + lenp, utf8, len);
		if(suffix)
			memcpy(fc + lenp + len, suffix, lens * sizeof *suffix);
		fc[lenp+len+lens] = 0;
	}
	return fc;
}

char *fc_to_utf8_alloc(uint16_t const *fc)
{
	size_t len = fc_len(fc);
	char *utf8 = malloc(len+1);
	if(utf8 != NULL)
		fc_to_utf8(utf8, fc, len+1);
	return utf8;
}

/* Split a path into components. Only the top array needs to be freed */
char **fs_split_components(char *path, int *count)
{
	char **comps = NULL;
	*count = 0;
	int allocated = 0;

	char *token = strtok(path, "/");

	while(token) {
		if(*count + 1 > allocated) {
			allocated += 8;
			char **new_comps = realloc(comps,
				allocated * sizeof *comps);
			if(!new_comps) {
				*count = -1;
				return NULL;
			}
			comps = new_comps;
		}
		comps[*count] = token;
		*count += 1;

		token = strtok(NULL, "/");
	}

	return comps;
}

/* Generalization of fs_path_normalize(). Returns a [char *] if use_fc=false,
   otherwise returns an [uint16_t *]. */
static void *path_normalize(char const *path, bool use_fc)
{
	char *path_dup = strdup(path);
	if(!path_dup) return NULL;

	int components=0;
	char **comps = fs_split_components(path_dup, &components);
	if(components == -1) {
		free(path_dup);
		return NULL;
	}

	/* Now rewrite comps[] to skip "." and apply ".." */
	int wr = 0;
	for(int rd=0; rd < components; rd++) {
		char *comp = comps[rd];
		if(!strcmp(comp, "."))
			continue;
		else if(!strcmp(comp, ".."))
			wr -= (wr > 0);
		else
			comps[wr++] = comp;
	}

	/* Count total length */
	int length = (use_fc ? 7 : 1) + (wr >= 1 ? wr - 1 : 0);
	for(int i = 0; i < wr; i++) {
		length += utf8_len(comps[i]);
	}

	/* Allocate string then copy */
	if(use_fc) {
		uint16_t *fc = malloc((length + 1) * sizeof *fc);
		uint16_t *fc_init = fc;

		memcpy(fc, u"\\\\fls0\\", 7*2);
		fc += 7;

		for(int i = 0; i < wr; i++) {
			if(i > 0) *fc++ = '\\';
			utf8_to_fc(fc, comps[i], (size_t)-1);
			fc += utf8_len(comps[i]);
		}

		*fc++ = 0;
		free(path_dup);
		free(comps);
		return fc_init;
	}
	else {
		char *utf8 = malloc(length + 1);
		char *utf8_init = utf8;
		*utf8++ = '/';

		for(int i = 0; i < wr; i++) {
			if(i > 0) *utf8++ = '/';
			strcpy(utf8, comps[i]);
			utf8 += utf8_len(comps[i]);
		}

		*utf8++ = 0;
		free(path_dup);
		free(comps);
		return utf8_init;
	}
}

char *fs_path_normalize(char const *path)
{
	return path_normalize(path, false);
}

uint16_t *fs_path_normalize_fc(char const *path)
{
	return path_normalize(path, true);
}
