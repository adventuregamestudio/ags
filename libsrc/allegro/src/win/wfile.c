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
 *      Helper routines to make file.c work on Windows platforms.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <time.h>
   #include <sys/stat.h>
#endif

#include "allegro.h"
#include "winalleg.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


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


/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive. This really only applies to the DOS library, so we don't bother
 *  with it.
 */
int _al_file_isok(AL_CONST char *filename)
{
   return TRUE;
}



/* _al_drive_exists:
 *  Checks whether the specified drive is valid.
 */
int _al_drive_exists(int drive)
{
   return GetLogicalDrives() & (1 << drive);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive(void)
{
   return _getdrive() - 1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[1024];

   if (get_filename_encoding() != U_UNICODE) {
      if (_getdcwd(drive+1, tmp, sizeof(tmp)))
         do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
      else
         usetc(buf, 0);
   }
   else {
      if (_wgetdcwd(drive+1, (wchar_t*)tmp, sizeof(tmp)/sizeof(wchar_t)))
         do_uconvert(tmp, U_UNICODE, buf, U_CURRENT, size);
      else
         usetc(buf, 0);
   }
}



/* _al_win_open:
 *  Open a file with open() or _wopen() depending on whether Unicode filenames
 *  are supported by this version of Windows and compiler.
 */
int _al_win_open(const char *filename, int mode, int perm)
{
   if (get_filename_encoding() != U_UNICODE) {
      return open(filename, mode, perm);
   }
   else {
      return _wopen((wchar_t*)filename, mode, perm);
   }
}
