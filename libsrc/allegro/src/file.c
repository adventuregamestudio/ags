/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      File I/O.
 *
 *      By Shawn Hargreaves.
 *
 *      _pack_fdopen() and related modifications by Annie Testes.
 *
 *      Evert Glebbeek added the support for relative filenames:
 *      make_absolute_filename(), make_relative_filename() and
 *      is_relative_filename().
 *
 *      Peter Wang added support for packfile vtables.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_MPW
   #include <sys/stat.h>
#endif

#if defined (ALLEGRO_UNIX) || defined (ALLEGRO_MACOSX)
   #include <pwd.h>                 /* for tilde expansion */
#endif

#ifdef ALLEGRO_WINDOWS
   #include "winalleg.h" /* for GetTempPath */
#endif

#ifndef O_BINARY
   #define O_BINARY  0
#endif

/* permissions to use when opening files */
#ifndef ALLEGRO_MPW

/* some OSes have no concept of "group" and "other" */
#ifndef S_IRGRP
   #define S_IRGRP   0
   #define S_IWGRP   0
#endif
#ifndef S_IROTH
   #define S_IROTH   0
   #define S_IWOTH   0
#endif

#define OPEN_PERMS   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#endif /* !ALLEGRO_MPW */



static char the_password[256] = EMPTY_STRING;

int _packfile_filesize = 0;
int _packfile_datasize = 0;

int _packfile_type = 0;

static PACKFILE_VTABLE normal_vtable;

static int filename_encoding = U_ASCII;





/***************************************************
 ****************** Path handling ******************
 ***************************************************/



/* Use strictly UTF-8 encoding for the file paths
*/
#define U_CURRENT U_UTF8
#define ugetc     utf8_getc
#define ugetx     utf8_getx
#define ugetxc    utf8_getx
#define usetc     utf8_setc
#define uwidth    utf8_width
#define ucwidth   utf8_cwidth
#define uisok     utf8_isok


/* fix_filename_case:
 *  Converts filename to upper case.
 */
char *fix_filename_case(char *filename)
{
   ASSERT(filename);
   
   if (!ALLEGRO_LFN)
      ustrupr(filename);

   return filename;
}



/* fix_filename_slashes:
 *  Converts '/' or '\' to system specific path separator.
 */
char *fix_filename_slashes(char *filename)
{
   int pos, c;
   ASSERT(filename);

   for (pos=0; ugetc(filename+pos); pos+=uwidth(filename+pos)) {
      c = ugetc(filename+pos);
      if ((c == '/') || (c == '\\'))
	 usetat(filename+pos, 0, OTHER_PATH_SEPARATOR);
   }

   return filename;
}



/* Canonicalize_filename:
 *  Returns the canonical form of the specified filename, i.e. the
 *  minimal absolute filename describing the same file.
 */
char *canonicalize_filename(char *dest, AL_CONST char *filename, int size)
{
   int saved_errno = errno;
   char buf[1024], buf2[1024];
   char *p;
   int pos = 0;
   int drive = -1;
   int c1, i;
   ASSERT(dest);
   ASSERT(filename);
   ASSERT(size >= 0);

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      /* check whether we have a drive letter */
      c1 = utolower(ugetc(filename));
      if ((c1 >= 'a') && (c1 <= 'z')) {
	 int c2 = ugetat(filename, 1);
	 if (c2 == DEVICE_SEPARATOR) {
	    drive = c1 - 'a';
	    filename += uwidth(filename);
	    filename += uwidth(filename);
	 }
      }

      /* if not, use the current drive */
      if (drive < 0)
	 drive = _al_getdrive();

      pos += usetc(buf+pos, drive+'a');
      pos += usetc(buf+pos, DEVICE_SEPARATOR);

   #endif

   #if defined (ALLEGRO_UNIX) || defined (ALLEGRO_MACOSX)

      /* if the filename starts with ~ then it's relative to a home directory */
      if ((ugetc(filename) == '~')) {
	 AL_CONST char *tail = filename + uwidth(filename); /* could be the username */
	 char *home = NULL;                /* their home directory */

	 if (ugetc(tail) == '/' || !ugetc(tail)) {
	    /* easy */
	    home = getenv("HOME");
	    if (home)
	       home = _al_strdup(home);
	 }
	 else {
	    /* harder */
	    char *username = (char *)tail, *ascii_username, *ch;
	    int userlen;
	    struct passwd *pwd;

	    /* find the end of the username */
	    tail = ustrchr(username, '/');
	    if (!tail)
	       tail = ustrchr(username, '\0');

	    /* this ought to be the ASCII length, but I can't see a Unicode
	     * function to return the difference in characters between two
	     * pointers. This code is safe on the assumption that ASCII is
	     * the most efficient encoding, but wasteful of memory */
	    userlen = tail - username + ucwidth('\0');
	    ascii_username = _AL_MALLOC_ATOMIC(userlen);

	    if (ascii_username) {
	       /* convert the username to ASCII, find the password entry,
		* and copy their home directory. */
	       do_uconvert(username, U_CURRENT, ascii_username, U_ASCII, userlen);

	       if ((ch = strchr(ascii_username, '/')))
		  *ch = '\0';

	       setpwent();

	       while (((pwd = getpwent()) != NULL) && 
		      (strcmp(pwd->pw_name, ascii_username) != 0))
		  ;

	       _AL_FREE(ascii_username);

	       if (pwd)
		  home = _al_strdup(pwd->pw_dir);

	       endpwent();
	    }
	 }

	 /* If we got a home directory, prepend it to the filename. Otherwise
	  * we leave the filename alone, like bash but not tcsh; bash is better
	  * anyway. :)
	  */
	 if (home) {
	    do_uconvert(home, U_ASCII, buf+pos, U_CURRENT, sizeof(buf)-pos);
	    _AL_FREE(home);
	    pos = ustrsize(buf);
	    filename = tail;
	    goto no_relativisation;
	 }
      }

   #endif   /* Unix */

   /* if the filename is relative, make it absolute */
   if ((ugetc(filename) != '/') && (ugetc(filename) != OTHER_PATH_SEPARATOR) && (ugetc(filename) != '#')) {
      _al_getdcwd(drive, buf2, sizeof(buf2) - ucwidth(OTHER_PATH_SEPARATOR));
      put_backslash(buf2);

      p = buf2;
      if ((utolower(p[0]) >= 'a') && (utolower(p[0]) <= 'z') && (p[1] == DEVICE_SEPARATOR))
	 p += 2;

      ustrzcpy(buf+pos, sizeof(buf)-pos, p);
      pos = ustrsize(buf);
   }

 #if defined (ALLEGRO_UNIX) || defined (ALLEGRO_MACOSX)
   no_relativisation:
 #endif

   /* add our filename, and clean it up a bit */
   ustrzcpy(buf+pos, sizeof(buf)-pos, filename);

   fix_filename_case(buf);
   fix_filename_slashes(buf);

   /* remove duplicate slashes */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL)
      uremove(p, 0);

   /* remove /./ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      uremove(p, 0);
      uremove(p, 0);
   }

   /* collapse /../ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      for (i=0; buf+uoffset(buf, i) < p; i++)
	 ;

      while (--i > 0) {
	 c1 = ugetat(buf, i);

	 if (c1 == OTHER_PATH_SEPARATOR)
	    break;

	 if (c1 == DEVICE_SEPARATOR) {
	    i++;
	    break;
	 }
      }

      if (i < 0)
	 i = 0;

      p += ustrsize(buf2);
      memmove(buf+uoffset(buf, i+1), p, ustrsizez(p));
   }

   /* all done! */
   ustrzcpy(dest, size, buf);

   errno = saved_errno;

   return dest;
}



/* make_relative_filename:
 *  Makes the relative filename corresponding to the specified absolute
 *  filename using the specified base (PATH is absolute and represents
 *  the base, FILENAME is the absolute filename), stores it in DEST
 *  whose size in bytes is SIZE and returns a pointer to it, or returns
 *  NULL if it cannot do so.
 *  It does not append '/' to the path.
 */
char *make_relative_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char *my_path, *my_filename;
   char *reduced_path = NULL, *reduced_filename = NULL;
   char *p1, *p2;
   int c, c1, c2, pos;
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   /* The first check under DOS/Windows would be for the drive: since the
    * paths are absolute, they will always contain a drive letter. Do this
    * check under Unix too where the first character should always be '/'
    * in order not to screw up existing DOS/Windows paths.
    */
   if (ugetc(path) != ugetc(filename))
      return NULL;

   my_path = _al_ustrdup(path);
   if (!my_path)
      return NULL;

   my_filename = _al_ustrdup(filename);
   if (!my_filename) {
      _AL_FREE(my_path);
      return NULL;
   }

   /* Strip the filenames to keep only the directories. */
   usetc(get_filename(my_path), 0);
   usetc(get_filename(my_filename), 0);

   /* Both paths are on the same device. There are three cases:
    *  - the filename is a "child" of the path in the directory tree,
    *  - the filename is a "brother" of the path,
    *  - the filename is only a "cousin" of the path.
    * In the two former cases, we will only need to keep a suffix of the
    * filename. In the latter case, we will need to back-paddle through
    * the directory tree.
    */
   p1 = my_path;
   p2 = my_filename;
   while (((c1=ugetx(&p1)) == (c2=ugetx(&p2))) && c1 && c2) {
      if ((c1 == '/') || (c1 == OTHER_PATH_SEPARATOR)) {
	 reduced_path = p1;
	 reduced_filename = p2;
      }
   }

   if (!c1) {
      /* If the path is exhausted, we are in one of the two former cases. */

      if (!c2) {
	 /* If the filename is also exhausted, we are in the second case.
	  * Prepend './' to the reduced filename.
	  */
	 pos = usetc(dest, '.');
	 pos += usetc(dest+pos, OTHER_PATH_SEPARATOR);
	 usetc(dest+pos, 0);
      }
      else {
	 /* Otherwise we are in the first case. Nothing to do. */
	 usetc(dest, 0);
      }
   }
   else {
      /* Bail out if previously something went wrong (eg. user supplied
       * paths are not canonical and we can't understand them). */
      if (!reduced_path) {
	 _AL_FREE(my_path);
	 _AL_FREE(my_filename);
	 return NULL;
      }
      /* Otherwise, we are in the latter case and need to count the number
       * of remaining directories in the reduced path and prepend the same
       * number of '../' to the reduced filename.
       */
      pos = 0;
      while ((c=ugetx(&reduced_path))) {
	 if ((c == '/') || (c == OTHER_PATH_SEPARATOR)) {
	    pos += usetc(dest+pos, '.');
	    pos += usetc(dest+pos, '.');
	    pos += usetc(dest+pos, OTHER_PATH_SEPARATOR);
	 }
      }

      usetc(dest+pos, 0);
   }

   /* Bail out if previously something went wrong (eg. user supplied
    * paths are not canonical and we can't understand them). */
   if (!reduced_filename) {
      _AL_FREE(my_path);
      _AL_FREE(my_filename);
      return NULL;
   }

   ustrzcat(dest, size, reduced_filename);
   ustrzcat(dest, size, get_filename(filename));

   _AL_FREE(my_path);
   _AL_FREE(my_filename);

   /* Harmonize path separators. */
   return fix_filename_slashes(dest);
}



/* is_relative_filename:
 *  Checks whether the specified filename is relative.
 */
int is_relative_filename(AL_CONST char *filename)
{
   ASSERT(filename);

   /* All filenames that start with a '.' are relative. */
   if (ugetc(filename) == '.')
      return TRUE;

   /* Filenames that contain a device separator (DOS/Windows)
    * or start with a '/' (Unix) are considered absolute.
    */
#if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
   if (ustrchr(filename, DEVICE_SEPARATOR)) 
      return FALSE;
#endif

   if ((ugetc(filename) == '/') || (ugetc(filename) == OTHER_PATH_SEPARATOR))
      return FALSE;

   return TRUE;
}



/* append_filename:
 *  Append filename to path, adding separator if necessary.
 */
char *append_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   int pos, c;
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   ustrzcpy(tmp, sizeof(tmp), path);
   pos = ustrlen(tmp);

   if ((pos > 0) && (uoffset(tmp, pos) < ((int)sizeof(tmp) - ucwidth(OTHER_PATH_SEPARATOR) - ucwidth(0)))) {
      c = ugetat(tmp, pos-1);

      if ((c != '/') && (c != OTHER_PATH_SEPARATOR) && (c != DEVICE_SEPARATOR)) {
	 pos = uoffset(tmp, pos);
	 pos += usetc(tmp+pos, OTHER_PATH_SEPARATOR);
	 usetc(tmp+pos, 0);
      }
   }

   ustrzcat(tmp, sizeof(tmp), filename);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion. Both '\' and '/' are recognized as directory
 *  separators.
 */
char *get_filename(AL_CONST char *path)
{
   int c;
   const char *ptr, *ret;
   ASSERT(path);

   ptr = path;
   ret = ptr;
   for (;;) {
      c = ugetxc(&ptr);
      if (!c) break;
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
         ret = (char*)ptr;
   }
   return (char*)ret;
}



/* get_extension:
 *  When passed a complete filename (with or without path information)
 *  this returns a pointer to the file extension.
 */
char *get_extension(AL_CONST char *filename)
{
   int pos, c;
   ASSERT(filename);

   pos = ustrlen(filename);

   while (pos>0) {
      c = ugetat(filename, pos-1);
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
	 break;
      pos--;
   }

   if ((pos>0) && (ugetat(filename, pos-1) == '.'))
      return (char *)filename + uoffset(filename, pos);

   return (char *)filename + ustrsize(filename);
}



/* put_backslash:
 *  If the last character of the filename is not a \, /, or #, or a device
 *  separator (eg. : under DOS), this routine will concatenate a \ or / on
 *  to it (depending on platform).
 */
void put_backslash(char *filename)
{
   int c;
   ASSERT(filename);

   if (ugetc(filename)) {
      c = ugetat(filename, -1);

      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
	 return;
   }

   filename += ustrsize(filename);
   filename += usetc(filename, OTHER_PATH_SEPARATOR);
   usetc(filename, 0);
}



/***************************************************
 ******************* Filesystem ********************
 ***************************************************/



/* set_filename_encoding:
 *  Sets the encoding to use for filenames. By default, UTF8 is assumed.
 */
void set_filename_encoding(int encoding)
{
    filename_encoding = encoding;
}



/* get_filename_encoding:
 *  Returns the encoding currently assumed for filenames.
 */
int get_filename_encoding(void)
{
    return filename_encoding ;
}



/***************************************************
 ******************** Packfiles ********************
 ***************************************************/


/* pack_fopen_datafile_object:
 *  Recursive helper to handle opening member objects from datafiles, 
 *  given a fake filename in the form 'object_name[/nestedobject]'.
 */
static PACKFILE *pack_fopen_datafile_object(PACKFILE *f, AL_CONST char *objname)
{
   char buf[512];   /* text is read into buf as UTF-8 */
   char tmp[512*4]; /* this should be enough even when expanding to UCS-4 */
   char name[512];
   int use_next = FALSE;
   int recurse = FALSE;
   int type, size, pos, c;

   /* split up the object name */
   pos = 0;

   while ((c = ugetxc(&objname)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR)) {
	 recurse = TRUE;
	 break;
      }
      pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   pack_mgetl(f);

   /* search for the requested object */
   while (!pack_feof(f)) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
	 type = pack_mgetl(f);
	 size = pack_mgetl(f);
	 if (type == DAT_NAME) {
	    /* examine name property */
	    pack_fread(buf, size, f);
	    buf[size] = 0;
	    if (ustricmp(uconvert(buf, U_UTF8, tmp, U_CURRENT, sizeof tmp), name) == 0)
	       use_next = TRUE;
	 }
	 else {
	    /* skip property */
	    pack_fseek(f, size);
	 }
      }
      else {
	 if (use_next) {
	    /* found it! */
	    if (recurse) {
	       if (type == DAT_FILE)
		  return pack_fopen_datafile_object(pack_fopen_chunk(f, FALSE), objname);
	       else
		  break;
	    }
	    else {
	       _packfile_type = type;
	       return pack_fopen_chunk(f, FALSE);
	    }
	 }
	 else {
	    /* skip unwanted object */
	    size = pack_mgetl(f);
	    pack_fseek(f, size+4);
	 }
      }
   }

   /* oh dear, the object isn't there... */
   pack_fclose(f);
   *allegro_errno = ENOENT;
   return NULL; 
}



/* create_packfile:
 *  Helper function for creating a PACKFILE structure.
 */
static PACKFILE *create_packfile(int is_normal_packfile)
{
   PACKFILE *f;

   if (is_normal_packfile)
      f = _AL_MALLOC(sizeof(PACKFILE));
   else
      f = _AL_MALLOC(sizeof(PACKFILE) - sizeof(struct _al_normal_packfile_details));

   if (f == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   if (!is_normal_packfile) {
      f->vtable = NULL;
      f->userdata = NULL;
      f->is_normal_packfile = FALSE;
   }
   else {
      f->vtable = &normal_vtable;
      f->userdata = f;
      f->is_normal_packfile = TRUE;

      f->normal.buf_pos = f->normal.buf;
      f->normal.flags = 0;
      f->normal.buf_size = 0;
      f->normal.filename = NULL;
      f->normal.parent = NULL;
      f->normal.todo = 0;
   }

   return f;
}



/* free_packfile:
 *  Helper function for freeing the PACKFILE struct.
 */
static void free_packfile(PACKFILE *f)
{
   if (f) {
      /* These are no longer the responsibility of this function, but
       * these assertions help catch instances of old code which still
       * rely on the old behaviour.
       */
      if (f->is_normal_packfile) {
	 ASSERT(!f->normal.pack_data);
	 ASSERT(!f->normal.unpack_data);
	 ASSERT(!f->normal.passdata);
	 ASSERT(!f->normal.passpos);
      }

      _AL_FREE(f);
   }
}



/* _pack_fdopen:
 *  Converts the given file descriptor into a PACKFILE. The mode can have
 *  the same values as for pack_fopen() and must be compatible with the
 *  mode of the file descriptor. Unlike the libc fdopen(), pack_fdopen()
 *  is unable to convert an already partially read or written file (i.e.
 *  the file offset must be 0).
 *  On success, it returns a pointer to a file structure, and on error it
 *  returns NULL and stores an error code in errno. An attempt to read
 *  a normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *_pack_fdopen(int fd, AL_CONST char *mode)
{
   PACKFILE *f;
   int c;

   if ((f = create_packfile(TRUE)) == NULL)
      return NULL;

   ASSERT(f->is_normal_packfile);

   while ((c = *(mode++)) != 0) {
      switch (c) {
	 case 'r': case 'R': f->normal.flags &= ~PACKFILE_FLAG_WRITE; break;
	 case 'w': case 'W': f->normal.flags |= PACKFILE_FLAG_WRITE; break;
      }
   }

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {
     /* write a 'real' file */
     f->normal.hndl = fd;
     f->normal.todo = 0;
     errno = 0;
   }
   else {
	 /* read a 'real' file */
	 f->normal.todo = lseek(fd, 0, SEEK_END);  /* size of the file */
	 if (f->normal.todo < 0) {
	    *allegro_errno = errno;
	    free_packfile(f);
	    return NULL;
     }

     lseek(fd, 0, SEEK_SET);
     f->normal.hndl = fd;
   }

   return f;
}



/* pack_fopen:
 *  Opens a file according to mode, which may contain any of the flags:
 *  'r': open file for reading.
 *  'w': open file for writing, overwriting any existing data.
 *  'p': open file in 'packed' mode. Data will be compressed as it is
 *       written to the file, and automatically uncompressed during read
 *       operations. Files created in this mode will produce garbage if
 *       they are read without this flag being set.
 *  '!': open file for writing in normal, unpacked mode, but add the value
 *       F_NOPACK_MAGIC to the start of the file, so that it can be opened
 *       in packed mode and Allegro will automatically detect that the
 *       data does not need to be decompressed.
 *
 *  Instead of these flags, one of the constants F_READ, F_WRITE,
 *  F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the second 
 *  argument to fopen().
 *
 *  On success, fopen() returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read a 
 *  normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(AL_CONST char *filename, AL_CONST char *mode)
{
   char tmp[1024];
   int fd;
   ASSERT(filename);

   _packfile_type = 0;

   if (!_al_file_isok(filename))
      return NULL;

   if (strpbrk(mode, "wW"))  /* write mode? */
      fd = _al_open(uconvert_tofilename(filename, tmp), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, OPEN_PERMS);
   else
      fd = _al_open(uconvert_tofilename(filename, tmp), O_RDONLY | O_BINARY, OPEN_PERMS);

   if (fd < 0) {
      *allegro_errno = errno;
      return NULL;
   }

   return _pack_fdopen(fd, mode);
}



/* pack_fopen_vtable:
 *  Creates a new packfile structure that uses the functions specified in
 *  the vtable instead of the standard functions.  On success, it returns a
 *  pointer to a file structure, and on error it returns NULL and
 *  stores an error code in errno.
 *
 *  The vtable and userdata must remain available for the lifetime of the
 *  created packfile.
 *
 *  Opening chunks using pack_fopen_chunk() on top of the returned packfile
 *  is not possible at this time.
 *
 *  packfile_password() does not have any effect on packfiles opened
 *  with pack_fopen_vtable().
 */
PACKFILE *pack_fopen_vtable(AL_CONST PACKFILE_VTABLE *vtable, void *userdata)
{
   PACKFILE *f;
   ASSERT(vtable);
   ASSERT(vtable->pf_fclose);
   ASSERT(vtable->pf_getc);
   ASSERT(vtable->pf_ungetc);
   ASSERT(vtable->pf_fread);
   ASSERT(vtable->pf_putc);
   ASSERT(vtable->pf_fwrite);
   ASSERT(vtable->pf_fseek);
   ASSERT(vtable->pf_feof);
   ASSERT(vtable->pf_ferror);

   if ((f = create_packfile(FALSE)) == NULL)
      return NULL;

   f->vtable = vtable;
   f->userdata = userdata;
   ASSERT(!f->is_normal_packfile);

   return f;
}



/* pack_fclose:
 *  Closes a file after it has been read or written.
 *  Returns zero on success. On error it returns an error code which is
 *  also stored in errno. This function can fail only when writing to
 *  files: if the file was opened in read mode it will always succeed.
 */
int pack_fclose(PACKFILE *f)
{
   int ret;

   if (!f)
      return 0;

   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_fclose);

   ret = f->vtable->pf_fclose(f->userdata);
   if (ret != 0)
      *allegro_errno = errno;

   free_packfile(f);

   return ret;
}



/* pack_fopen_chunk: 
 *  Opens a sub-chunk of the specified file, for reading or writing depending
 *  on the type of the file. The returned file pointer describes the sub
 *  chunk, and replaces the original file, which will no longer be valid.
 *  When writing to a chunk file, data is sent to the original file, but
 *  is prefixed with two length counts (32 bit, big-endian). For uncompressed 
 *  chunks these will both be set to the length of the data in the chunk.
 *  For compressed chunks, created by setting the pack flag, the first will
 *  contain the raw size of the chunk, and the second will be the negative
 *  size of the uncompressed data. When reading chunks, the pack flag is
 *  ignored, and the compression type is detected from the sign of the
 *  second size value. The file structure used to read chunks checks the
 *  chunk size, and will return EOF if you try to read past the end of
 *  the chunk. If you don't read all of the chunk data, when you call
 *  pack_fclose_chunk(), the parent file will advance past the unused data.
 *  When you have finished reading or writing a chunk, you should call
 *  pack_fclose_chunk() to return to your original file.
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
   PACKFILE *chunk;
   ASSERT(f);

   /* unsupported */
   if (!f->is_normal_packfile) {
      *allegro_errno = EINVAL;
      return NULL;
   }

   {
      /* read a sub-chunk */
      _packfile_filesize = pack_mgetl(f);
      _packfile_datasize = pack_mgetl(f);

      if ((chunk = create_packfile(TRUE)) == NULL)
         return NULL;

      chunk->normal.flags = PACKFILE_FLAG_CHUNK;
      chunk->normal.parent = f;

      ASSERT(_packfile_datasize >= 0);
      {
	 /* read an uncompressed chunk */
	 chunk->normal.todo = _packfile_datasize;
      }
   }

   return chunk;
}



/* pack_fseek:
 *  Like the stdio fseek() function, but only supports forward seeks 
 *  relative to the current file position.
 */
int pack_fseek(PACKFILE *f, int offset)
{
   ASSERT(f);
   ASSERT(offset >= 0);

   return f->vtable->pf_fseek(f->userdata, offset);
}



/* pack_getc:
 *  Returns the next character from the stream f, or EOF if the end of the
 *  file has been reached.
 */
int pack_getc(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_getc);

   return f->vtable->pf_getc(f->userdata);
}



/* pack_putc:
 *  Puts a character in the stream f.
 */
int pack_putc(int c, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_putc);

   return f->vtable->pf_putc(c, f->userdata);
}



/* pack_feof:
 *  pack_feof() returns nonzero as soon as you reach the end of the file. It 
 *  does not wait for you to attempt to read beyond the end of the file,
 *  contrary to the ISO C feof() function.
 */
int pack_feof(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_feof);

   return f->vtable->pf_feof(f->userdata);
}



/* pack_igetw:
 *  Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b2 << 8) | b1);

   return EOF;
}



/* pack_igetl:
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b4 << 24) | ((long)b3 << 16) |
		       ((long)b2 << 8) | (long)b1);

   return EOF;
}



/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
	 return w;

   return EOF;
}



/* pack_iputl:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
	 if (pack_putc(b2,f)==b2)
	    if (pack_putc(b1,f)==b1)
	       return l;

   return EOF;
}



/* pack_mgetw:
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 return ((b1 << 8) | b2);

   return EOF;
}



/* pack_mgetl:
 *  Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((long)b1 << 24) | ((long)b2 << 16) |
		       ((long)b3 << 8) | (long)b4);

   return EOF;
}



/* pack_fread:
 *  Reads n bytes from f and stores them at memory location p. Returns the 
 *  number of items read, which will be less than n if EOF is reached or an 
 *  error occurs. Error codes are stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_fread);
   ASSERT(p);
   ASSERT(n >= 0);

   return f->vtable->pf_fread(p, n, f->userdata);
}



/* pack_fputs:
 *  Writes a string to a text file, returning zero on success, -1 on error.
 *  The input string is converted from the current text encoding format
 *  to UTF-8 before writing. Newline characters (\n) are written as \r\n
 *  on DOS and Windows platforms.
 */
int pack_fputs(AL_CONST char *p, PACKFILE *f)
{
   char *buf, *s;
   int bufsize;
   ASSERT(f);
   ASSERT(p);

   *allegro_errno = 0;

   bufsize = uconvert_size(p, U_CURRENT, U_UTF8);
   buf = _AL_MALLOC_ATOMIC(bufsize);
   if (!buf)
      return -1;

   s = uconvert(p, U_CURRENT, buf, U_UTF8, bufsize);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
	 if (*s == '\n')
	    pack_putc('\r', f);
      #endif

      pack_putc(*s, f);
      s++;
   }

   _AL_FREE(buf);

   if (*allegro_errno)
      return -1;
   else
      return 0;
}



/***************************************************
 ************ "Normal" packfile vtable *************
 ***************************************************

   Ideally this would be the only section which knows about the details
   of "normal" packfiles. However, this ideal is still being violated in
   many places (partly due to the API, and partly because it would be
   quite a lot of work to move the _al_normal_packfile_details field
   into the userdata field of the PACKFILE structure.
*/


static int normal_fclose(void *_f);
static int normal_getc(void *_f);
static int normal_ungetc(int ch, void *_f);
static int normal_putc(int c, void *_f);
static long normal_fread(void *p, long n, void *_f);
static long normal_fwrite(AL_CONST void *p, long n, void *_f);
static int normal_fseek(void *_f, int offset);
static int normal_feof(void *_f);
static int normal_ferror(void *_f);

static int normal_refill_buffer(PACKFILE *f);
static int normal_flush_buffer(PACKFILE *f, int last);



static PACKFILE_VTABLE normal_vtable =
{
   normal_fclose,
   normal_getc,
   normal_ungetc,
   normal_fread,
   normal_putc,
   normal_fwrite,
   normal_fseek,
   normal_feof,
   normal_ferror
};



static int normal_fclose(void *_f)
{
   PACKFILE *f = _f;
   int ret;

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {
      normal_flush_buffer(f, TRUE);
   }

   if (f->normal.parent) {
      ret = pack_fclose(f->normal.parent);
   }
   else {
      ret = close(f->normal.hndl);
      if (ret != 0)
	 *allegro_errno = errno;
   }

   return ret;
}



/* normal_no_more_input:
 *  Return non-zero if the number of bytes remaining in the file (todo) is
 *  less than or equal to zero.
 *
 *  However, there is a special case.  If we are reading from a LZSS
 *  compressed file, the latest call to lzss_read() may have suspended while
 *  writing out a sequence of bytes due to the output buffer being too small.
 *  In that case the `todo' count would be decremented (possibly to zero),
 *  but the output isn't yet completely written out.
 */
static INLINE int normal_no_more_input(PACKFILE *f)
{
   return (f->normal.todo <= 0);
}



static int normal_getc(void *_f)
{
   PACKFILE *f = _f;

   f->normal.buf_size--;
   if (f->normal.buf_size > 0)
      return *(f->normal.buf_pos++);

   if (f->normal.buf_size == 0) {
      if (normal_no_more_input(f))
	 f->normal.flags |= PACKFILE_FLAG_EOF;
      return *(f->normal.buf_pos++);
   }

   return normal_refill_buffer(f);
}



static int normal_ungetc(int c, void *_f)
{
   PACKFILE *f = _f;

   if (f->normal.buf_pos == f->normal.buf) {
      return EOF;
   }
   else {
      *(--f->normal.buf_pos) = (unsigned char)c;
      f->normal.buf_size++;
      f->normal.flags &= ~PACKFILE_FLAG_EOF;
      return (unsigned char)c;
   }
}



static long normal_fread(void *p, long n, void *_f)
{
   PACKFILE *f = _f;
   unsigned char *cp = (unsigned char *)p;
   long i;
   int c;

   for (i=0; i<n; i++) {
      if ((c = normal_getc(f)) == EOF)
	 break;

      *(cp++) = c;
   }

   return i;
}



static int normal_putc(int c, void *_f)
{
   PACKFILE *f = _f;

   if (f->normal.buf_size + 1 >= F_BUF_SIZE) {
      if (normal_flush_buffer(f, FALSE))
	 return EOF;
   }

   f->normal.buf_size++;
   return (*(f->normal.buf_pos++) = c);
}



static long normal_fwrite(AL_CONST void *p, long n, void *_f)
{
   PACKFILE *f = _f;
   AL_CONST unsigned char *cp = (AL_CONST unsigned char *)p;
   long i;

   for (i=0; i<n; i++) {
      if (normal_putc(*cp++, f) == EOF)
	 break;
   }

   return i;
}



static int normal_fseek(void *_f, int offset)
{
   PACKFILE *f = _f;
   int i;

   if (f->normal.flags & PACKFILE_FLAG_WRITE)
      return -1;

   *allegro_errno = 0;

   /* skip forward through the buffer */
   if (f->normal.buf_size > 0) {
      i = MIN(offset, f->normal.buf_size);
      f->normal.buf_size -= i;
      f->normal.buf_pos += i;
      offset -= i;
      if ((f->normal.buf_size <= 0) && normal_no_more_input(f))
	 f->normal.flags |= PACKFILE_FLAG_EOF;
   }

   /* need to seek some more? */
   if (offset > 0) {
      i = MIN(offset, f->normal.todo);

	 if (f->normal.parent) {
	    /* pass the seek request on to the parent file */
	    pack_fseek(f->normal.parent, i);
	 }
	 else {
	    /* do a real seek */
	    lseek(f->normal.hndl, i, SEEK_CUR);
	 }
	 f->normal.todo -= i;
	 if (normal_no_more_input(f))
	    f->normal.flags |= PACKFILE_FLAG_EOF;
   }

   if (*allegro_errno)
      return -1;
   else
      return 0;
}



static int normal_feof(void *_f)
{
   PACKFILE *f = _f;

   return (f->normal.flags & PACKFILE_FLAG_EOF);
}



static int normal_ferror(void *_f)
{
   PACKFILE *f = _f;

   return (f->normal.flags & PACKFILE_FLAG_ERROR);
}



/* normal_refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int normal_refill_buffer(PACKFILE *f)
{
   int i, sz, done, offset;

   if (f->normal.flags & PACKFILE_FLAG_EOF)
      return EOF;

   if (normal_no_more_input(f)) {
      f->normal.flags |= PACKFILE_FLAG_EOF;
      return EOF;
   }

   if (f->normal.parent) {
       {
	 f->normal.buf_size = pack_fread(f->normal.buf, MIN(F_BUF_SIZE, f->normal.todo), f->normal.parent);
      } 
      if (f->normal.parent->normal.flags & PACKFILE_FLAG_EOF)
	 f->normal.todo = 0;
      if (f->normal.parent->normal.flags & PACKFILE_FLAG_ERROR)
	 goto Error;
   }
   else {
      f->normal.buf_size = MIN(F_BUF_SIZE, f->normal.todo);

      offset = lseek(f->normal.hndl, 0, SEEK_CUR);
      done = 0;

      errno = 0;
      sz = read(f->normal.hndl, f->normal.buf, f->normal.buf_size);

      while (sz+done < f->normal.buf_size) {
	 if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	    goto Error;

	 if (sz > 0)
	    done += sz;

	 lseek(f->normal.hndl, offset+done, SEEK_SET);
         errno = 0;
	 sz = read(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
      }
   }

   f->normal.todo -= f->normal.buf_size;
   f->normal.buf_pos = f->normal.buf;
   f->normal.buf_size--;
   if (f->normal.buf_size <= 0)
      if (normal_no_more_input(f))
	 f->normal.flags |= PACKFILE_FLAG_EOF;

   if (f->normal.buf_size < 0)
      return EOF;
   else
      return *(f->normal.buf_pos++);

 Error:
   *allegro_errno = EFAULT;
   f->normal.flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/* normal_flush_buffer:
 *  Flushes a file buffer to the disk. The file must be open in write mode.
 */
static int normal_flush_buffer(PACKFILE *f, int last)
{
   int i, sz, done, offset;

   if (f->normal.buf_size > 0) {
       {

	 offset = lseek(f->normal.hndl, 0, SEEK_CUR);
	 done = 0;

	 errno = 0;
	 sz = write(f->normal.hndl, f->normal.buf, f->normal.buf_size);

	 while (sz+done < f->normal.buf_size) {
	    if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
	       goto Error;

	    if (sz > 0)
	       done += sz;

	    lseek(f->normal.hndl, offset+done, SEEK_SET);
	    errno = 0;
	    sz = write(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
	 }
      }
      f->normal.todo += f->normal.buf_size;
   }

   f->normal.buf_pos = f->normal.buf;
   f->normal.buf_size = 0;
   return 0;

 Error:
   *allegro_errno = EFAULT;
   f->normal.flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}
