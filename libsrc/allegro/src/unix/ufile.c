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
 *      Helper routines to make file.c work on Unix (POSIX) platforms.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

/* libc should use 64-bit for file sizes when possible */
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef ALLEGRO_HAVE_DIRENT_H
   #include <sys/types.h>
   #include <dirent.h>
   #define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
   /* Apparently all new systems have `dirent.h'. */
   #error ALLEGRO_HAVE_DIRENT_H not defined
#endif

#ifdef ALLEGRO_HAVE_SYS_TIME_H
  #include <sys/time.h>
#endif
#ifdef ALLEGRO_HAVE_TIME_H
  #include <time.h>
#endif

#define PREFIX_I "al-unix INFO: "

#define PREFIX_I "al-unix INFO: "


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
 *  drive.
 */
int _al_file_isok(AL_CONST char *filename)
{
   return TRUE;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[1024];

   if (getcwd(tmp, sizeof(tmp)))
      do_uconvert(tmp, U_UTF8, buf, U_CURRENT, size);
   else
      usetc(buf, 0);
}
