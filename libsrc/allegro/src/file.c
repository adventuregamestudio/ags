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

/* create_packfile:
 *  Helper function for creating a PACKFILE structure.
 */
static PACKFILE *create_packfile()
{
   PACKFILE *f;

   f = _AL_MALLOC(sizeof(PACKFILE));

   if (f == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   f->vtable = NULL;
   f->userdata = NULL;

   return f;
}



/* free_packfile:
 *  Helper function for freeing the PACKFILE struct.
 */
static void free_packfile(PACKFILE *f)
{
   if (f) {
      _AL_FREE(f);
   }
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
int32_t pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((int32_t)b4 << 24) | ((int32_t)b3 << 16) |
		       ((int32_t)b2 << 8) | (int32_t)b1);

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
int32_t pack_iputl(int32_t l, PACKFILE *f)
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
int32_t pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
	 if ((b3 = pack_getc(f)) != EOF)
	    if ((b4 = pack_getc(f)) != EOF)
	       return (((int32_t)b1 << 24) | ((int32_t)b2 << 16) |
		       ((int32_t)b3 << 8) | (int32_t)b4);

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
