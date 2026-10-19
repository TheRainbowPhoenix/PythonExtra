//---
// gint:bfile - BFile interface
//
// BFile is the OS's native interface to the filesystem. It has quite a bit of
// legacy, so it's not very easy to use. Please note that BFile cannot be used
// from within gint, so do a gint_world_switch() (<gint/gint.h>) to call BFile
// functions; otherwise the add-in is likely to crash.
//
// There are two different filesystems:
// * An older, in-house one made by CASIO and informally called CASIOWIN (which
//   is the name of the OS itself), which is incomplete, hard to use, and very
//   limiting, but also very fast.
// * A newer one, designed by Kyoto Software Research and called Fugue, which
//   is mostly complete and behaves as a regular filesystem, though very slow.
//
// You can detect which one is used by querying gint[HWFS] (<gint/hardware.h>):
// * HWFS_FUGUE is the newer one; it is found in:
//   - The fx-CG series (in France: Prizm and Graph 90+E)
//   - The fx G-III series (in France: Graph 35+E II)
// * HWFS_CASIOWIN is the older one; it is found in all older black-and-white
//   model that have a storage memory.
//
// Wherever Fugue is used, gint supports the Unix file API (open, read, write,
// etc) and the C99 standard file API (fopen, fread, fwrite, etc), so there is
// no need to call into BFile directly (you should still do a world switch
// before using these functions).
//
// You should only have to use BFile if you're dealing with the CASIOWIN
// filesystem. With Fugue, not only are the Unix/C99 APIs easier to use, but
// the meanings of arguments and return values in BFile calls also change
// compared to the CASIOWIN filesystem, so there's little to gain anyway.
//
// If you're here for the CASIOWIN filesystem, be aware of its limitations. In
// general reading files will work fine; expect no issues with that. Modifying
// files is where things get difficult. Keep the following in mind:
//
// * Non-standard meanings of arguments and return values (eg. BFile_Read()
//   returns the number of readable bytes between the new position and EOF).
// * Files must be created with a fixed size indicated in BFile_Create() and
//   are initialized with 0xff.
// * A write to a file can only replace 1's with 0's, meaning in practice a
//   file can only be written to once. (That's why it's created with 0xff.)
// * All writes must be of even size; writing an odd number of bytes can mess
//   up the file and confuse the filesystem.
// * Only one level of folders is supported; attempting to create second-level
//   subfolders results in weird recursive directories (don't do that).
//
// File paths in the OS use the non-standard FONTCHARACTER encoding, which is a
// 16-bit fixed-width encoding. Most users don't care about special characters;
// in GCC, you can get a 16-bit literal string with the u"" prefix, eg.
//   u"\\\\fls0\\data.bin"
// which is what you usually supply as the [uint16_t const *path] argument.
//
// All functions return nonnegative values on success. If no return value is
// described then a successful call returns 0. Unless specified otherwise, all
// functions can also return a negative error code as documented near the end
// of this header.
//---

#ifndef GINT_BFILE
#define GINT_BFILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//---
// Common file access functions
//---

/* Remove a file or folder (also works if the entry does not exist). */
int BFile_Remove(uint16_t const *path);

/* Rename a file (can move folders; Fugue only). */
int BFile_Rename(uint16_t const *oldpath, uint16_t const *newpath);

#define BFile_File    1
#define BFile_Folder  5

/* BFile_Create(): Create a new file or folder

   The file or directory must not exist. For a file the size pointer must point
   to the desired file size (which is fixed), for a folder it is ignored. With
   CASIOWIN this is the final size of the file. With Fugue the file can be
   resized dynamically and is usually created with initial size 0.

   @path  FONTCHARACTER file path
   @type  Entry type (BFile_File or BFile_Folder)
   @size  Pointer to file size if type is BFile_File, use NULL otherwise */
int BFile_Create(uint16_t const *path, int type, int *size);

#define BFile_ReadOnly        0x01
#define BFile_WriteOnly       0x02
#define BFile_ReadWrite       (BFile_ReadOnly | BFile_WriteOnly)
#define BFile_Share           0x80
#define BFile_ReadShare       (BFile_ReadOnly | BFile_Share)
#define BFile_ReadWriteShare  (BFile_ReadWrite | BFile_Share)

/* BFile_Open(): Open a file for reading or writing

   The file must exist, even when opening in writing mode. The meaning of the
   [Share] flag isn't clear; I believe it's simply ignored in the CASIOWIN
   filesystem.

   @path  FONTCHARACTER file path
   @mode  Desired access mode
   Returns a file descriptor (or a negative error code). */
int BFile_Open(uint16_t const *path, int mode);

/* Close an open file descriptor. */
int BFile_Close(int fd);

/* Get the size of an open file. */
int BFile_Size(int fd);

/* BFile_Write(): Write data to an open file

   WARNING: The CASIOWIN fs is known to become inconsistent if an odd number of
            bytes is written with BFile_Write(). Always keep it even!

   With CASIOWIN, returns the new file offset after writing (or an error code).
   With Fugue, returns the amount of data written (or an error code). */
int BFile_Write(int fd, void const *data, int even_size);

/* BFile_Read(): Read data from an open file

   The extra argument [whence] specifies where data is read from in the style
   of pread(2), and is supported with both filesystems.

   * If [whence >= 0], it is taken as an absolute location within the file;
   * If [whence == -1], BFile_Read() reads from the current position.

   With Fugue this function can read past end of file and return the requested
   amount of bytes even when the file does not have enough data to read that
   much. It seems that extra bytes read as zeros. Reading past the end does
   *not* extend the file size.

   With CASIOWIN, returns how much data can be read from the updated file
   offset (ie. how many bytes until end of file), or an error code.
   With Fugue, returns the amount of data read (or an error code). */
int BFile_Read(int handle, void *data, int size, int whence);

/* BFile_Seek(): Seek to a relative or absolute position within an open file

   With CASIOWIN, moves [offset] bytes relative to the current position, and
   returns how much data can be read from the new position. BFile_Seek(fd, 0)
   combined with BFile_Size(fd) can be used to determine the current position.

   With Fugue, moves to the absolute position [offset] within the file, and
   returns the amount of allocated space following the new position (usually
   larger than the amount of data until end-of-file because files are allocated
   in units of 4096 bytes). There is no way to seek relative to the current
   position unless the target is precomputed with BFile_GetPos(). */
int BFile_Seek(int fd, int offset);

/* BFile_GetPos(): Get the current position in an open file

   This call does not exist in the CASIOWIN interface, so this function always
   returns -1 on models with a CASIOWIN filesystem.

   This call exists in Fugue, however some Fugue models have their syscall list
   inherited from the CASIOWIN era and don't have an entry point for it (or if
   there's one it's escape scrutiny so far).

   * Prizm / Graph 90+E / fx-CG series: this function works as intended.
   * Graph 35+E II / G-III series: the call is missing, returns -1.

   For the latter models there is no easily reliable way of knowing the current
   position within an open file! */
int BFile_GetPos(int fd);

//---
// Error codes
//---

#define BFile_EntryNotFound       -1
#define BFile_IllegalParam        -2
#define BFile_IllegalPath         -3
#define BFile_DeviceFull          -4
#define BFile_IllegalDevice       -5
#define BFile_IllegalFilesystem   -6
#define BFile_IllegalSystem       -7
#define BFile_AccessDenied        -8
#define BFile_AlreadyLocked       -9
#define BFile_IllegalTaskID       -10
#define BFile_PermissionError     -11
#define BFile_EntryFull           -12
#define BFile_AlreadyExists       -13
#define BFile_ReadOnlyFile        -14
#define BFile_IllegalFilter       -15
#define BFile_EnumerateEnd        -16
#define BFile_DeviceChanged       -17
//#define BFile_NotRecordFile     -18     // Not used
#define BFile_IllegalSeekPos      -19
#define BFile_IllegalBlockFile    -20
//#define BFile_NoSuchDevice      -21     // Not used
//#define BFile_EndOfFile         -22     // Not used
#define BFile_NotMountDevice      -23
#define BFile_NotUnmountDevice    -24
#define BFile_CannotLockSystem    -25
#define BFile_RecordNotFound      -26
//#define BFile_NotDualRecordFile -27     // Not used
#define BFile_NoAlarmSupport      -28
#define BFile_CannotAddAlarm      -29
#define BFile_FileFindUsed        -30
#define BFile_DeviceError         -31
#define BFile_SystemNotLocked     -32
#define BFile_DeviceNotFound      -33
#define BFile_FileTypeMismatch    -34
#define BFile_NotEmpty            -35
#define BFile_BrokenSystemData    -36
#define BFile_MediaNotReady       -37
#define BFile_TooManyAlarms       -38
#define BFile_SameAlarmExists     -39
#define BFile_AccessSwapArea      -40
#define BFile_MultimediaCard      -41
#define BFile_CopyProtection      -42
#define BFile_IllegalFileData     -43

//---
// Search API
//
// The search API is described below. It's somewhat unreliable, with unclear
// semantics and a history of breaking in seemingly reasonable programs. One
// day probably we'll know how to use it properly and reliably.
//
// Note: always close search handles or trouble will ensue (eg. add-in
// discovery failing).
//---

struct BFile_FileInfo
{
	uint16_t index;
	uint16_t type;
	uint32_t file_size;
	/* Data size (file size minus the header) */
	uint32_t data_size;
	/* Is 0 if the file is complete */
	uint32_t property;
	/* Address of first fragment (do not use directly) */
	void *address;
};

#define BFile_Type_Directory  0x0000
#define BFile_Type_File       0x0001
#define BFile_Type_Addin      0x0002
#define BFile_Type_Eact       0x0003
#define BFile_Type_Language   0x0004
#define BFile_Type_Bitmap     0x0005
#define BFile_Type_MainMem    0x0006
#define BFile_Type_Temp       0x0007
#define BFile_Type_Dot        0x0008
#define BFile_Type_DotDot     0x0009
#define BFile_Type_Volume     0x000a
#define BFile_Type_Archived   0x0041

/* BFile_FindFirst(): Search the storage memory for paths matching a pattern

   This if for the storage memory only. There are only four search handles;
   make sure to close them, there is no automatic device to close them for you
   even after the add-in exists.

   Search is NOT case sensitive. '*' can be used as a wildcard, although it's
   unclear whether more than one '*' is supported, whether '*' can match in a
   directory name, whether multiple folders can be searched simultaneously, and
   whether directories can be matched.

   @pattern    FONTCHARACTER glob pattern
   @shandle    Will receive search handle (to use in BFile_FindNext/FindClose)
   @foundfile  Will receive FONTCHARACTER path of matching entry
   @fileinfo   Will receive metadata of matching entry

   On success, returns 0, stores the search handle in *shandle, and stores
   information about the first match in foundfile and fileinfo. The negative
   error code BFile_EntryNotFound should be interpreted as an empty result (ie.
   no entry matched) rather than an error.

   Returns 0 on success or a negative error code. BFile_EntryNotFound should be
   interpreted as an empty result (ie. no file matched). */
int BFile_FindFirst(uint16_t const *pattern, int *shandle, uint16_t *foundfile,
	struct BFile_FileInfo *fileinfo);

/* BFile_FindNext(): Continue a search

   Continues the search for matches. The search handle is the value set in
   *shandle in BFile_FindFirst(). As before, *foundfile receives the matching
   entry's path and *fileinfo its metadata.

   Returns 0 on success. The negative error code BFile_EnumerateEnd should be
   interpreted as the end of the search (ie. all matching files have been
   returned) rather than an error. */
int BFile_FindNext(int shandle, uint16_t *foundfile,
	struct BFile_FileInfo *fileinfo);

/* Close a search handle (with or without exhausting the matches). */
int BFile_FindClose(int shandle);

//---
// Additional functions
//
// The following functions are implemented in gint using lower-level BFile
// functions. They add abstraction/convenience to the API, but can be pretty
// slow as they result in more BFile calls being made.
//---

/* BFile_Ext_Stat(): Stat an entry for type and size */
int BFile_Ext_Stat(uint16_t const *path, int *type, int *size);

#ifdef __cplusplus
}
#endif

#endif /* GINT_BFILE */
