#ifndef FS_UTIL_H
#define FS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <gint/hardware.h>
#include <errno.h>

#define ENOTSUP_IF_NOT_FUGUE(rc) \
   if(gint[HWFS] != HWFS_FUGUE) { errno = ENOTSUP; return (rc); }

/* Translate common BFile error codes to errno values. */
int bfile_error_to_errno(int bfile_error_code);

/* Translate BFile file types to st_mode values. */
int bfile_type_to_mode_t(int bfile_type);

/* Translate BFile file types to struct dirent values. */
int bfile_type_to_dirent(int bfile_type);

/* TODO: These functions do not actually translate special characters between
   encodings, they simply strip them. */

/* Length of UTF-8 string _as copied by utf8_to_fc functions_ */
size_t utf8_len(char const *utf8);
/* Length of FONTCHARACTER string _as copied by fc_to_utf8 functions_ */
size_t fc_len(uint16_t const *fc);

/* Convert UTF-8 to FONTCHARACTER; outputs fc_len characters with padding. If
   fc[fc_len-1] is not 0 after the call, then fc is too short. */
void utf8_to_fc(uint16_t *fc, char const *utf8, size_t fc_len);

/* Same in the other direction. */
void fc_to_utf8(char *utf8, uint16_t const *fc, size_t utf8_len);

/* Same as utf8_to_fc() but allocates a string with malloc(). */
uint16_t *utf8_to_fc_alloc(uint16_t *prefix, char const *utf8,
   uint16_t *suffix);

/* Same as fc_to_utf8() but allocates a string with malloc(). */
char *fc_to_utf8_alloc(uint16_t const *fc);

#ifdef __cplusplus
}
#endif

#endif /* FS_UTIL_H */
