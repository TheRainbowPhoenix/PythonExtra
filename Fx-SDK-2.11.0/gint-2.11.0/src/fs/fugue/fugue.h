#ifndef FS_FUGUE_FUGUE_H
#define FS_FUGUE_FUGUE_H

#include <gint/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/* File descriptor type */
extern const fs_descriptor_type_t fugue_descriptor_type;
/* Directory descriptor type */
extern const fs_descriptor_type_t fugue_dir_descriptor_type;

/* Since on Graph 35+E II / fx-9860G III there is no (known) syscall to update
   the file position, we need to track it ourselves, hopefully faithfully. */
typedef struct {
    int fd;
    int pos;
} fugue_fd_t;

/* Specific implementations of some standard functions */

int fugue_open(char const *path, int flags, mode_t mode);

int fugue_unlink(char const *path);

int fugue_rename(char const *oldpath, char const *newpath);

int fugue_mkdir(char const *path, mode_t mode);

int fugue_rmdir(char const *path);

int fugue_stat(char const * restrict path, struct stat * restrict statbuf);

/* Other functions */

void *fugue_dir_explore(char const *path);

#endif /* FS_FUGUE_FUGUE_H */
