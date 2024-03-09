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
 *      Assorted globals and setup/cleanup routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



#define PREFIX_I                "al-main INFO: "
#define PREFIX_W                "al-main WARNING: "
#define PREFIX_E                "al-main ERROR: "

#ifdef ALLEGRO_AMIGA
   #define LOGFILE "RAM:allegro.log"
#else
   #define LOGFILE "allegro.log"
#endif


/* in case you want to report version numbers */
char allegro_id[] = "Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR;


/* error message for sound and gfx init routines */
char allegro_error[ALLEGRO_ERROR_SIZE] = EMPTY_STRING;


/* error value, which will work even with DLL linkage */
int *allegro_errno = NULL;


/* flag for how many times we have been initialised */
int _allegro_count = 0;


/* flag to know whether we are being called by the exit mechanism */
int _allegro_in_exit = FALSE;


/* info about the current graphics drawing mode */
int _drawing_mode = DRAW_MODE_SOLID;

BITMAP *_drawing_pattern = NULL;

int _drawing_x_anchor = 0;
int _drawing_y_anchor = 0;

unsigned int _drawing_x_mask = 0;
unsigned int _drawing_y_mask = 0;


/* default palette structures */
PALETTE black_palette;
PALETTE _current_palette; 

int _current_palette_changed = 0xFFFFFFFF;


/* colors for the standard GUI dialogs (alerts, file selector, etc) */
int gui_fg_color = 255;
int gui_mg_color = 8;
int gui_bg_color = 0;


/* a block of temporary working memory */
void *_scratch_mem = NULL;
int _scratch_mem_size = 0;

/* debugging stuff */
static int debug_assert_virgin = TRUE;
static int debug_trace_virgin = TRUE;

static FILE *assert_file = NULL;
static FILE *trace_file = NULL;

static int (*assert_handler)(AL_CONST char *msg) = NULL;
int (*_al_trace_handler)(AL_CONST char *msg) = NULL;


/* Module linking system stuff: if an object file is linked in, then its
 * constructor function is executed; this function should fill in the
 * data structures below (declared in aintern.h). If the module is not
 * linked in, then the structure pointers will be null, so we don't need
 * to bother with that bit of code elsewhere.
 */
struct _AL_LINKER_MIDI *_al_linker_midi = NULL;
struct _AL_LINKER_MOUSE *_al_linker_mouse = NULL;


/* dynamic registration system for cleanup code */
struct al_exit_func {
   void (*funcptr)(void);
   AL_CONST char *desc;
   struct al_exit_func *next;
};

static struct al_exit_func *exit_func_list = NULL;



/* _add_exit_func:
 *  Adds a function to the list that need to be called by allegro_exit().
 *  `desc' should point to a statically allocated string to help with
 *  debugging.
 */
void _add_exit_func(void (*func)(void), AL_CONST char *desc)
{
   struct al_exit_func *n;

   for (n = exit_func_list; n; n = n->next)
      if (n->funcptr == func)
	 return;

   n = _AL_MALLOC(sizeof(struct al_exit_func));
   if (!n)
      return;

   n->next = exit_func_list;
   n->funcptr = func;
   n->desc = desc;
   exit_func_list = n;
}



/* _remove_exit_func:
 *  Removes a function from the list that need to be called by allegro_exit().
 */
void _remove_exit_func(void (*func)(void))
{
   struct al_exit_func *iter = exit_func_list, *prev = NULL;

   while (iter) {
      if (iter->funcptr == func) {
	 if (prev)
	    prev->next = iter->next;
	 else
	    exit_func_list = iter->next;
	 _AL_FREE(iter);
	 return;
      }
      prev = iter;
      iter = iter->next;
   }
}



/* allegro_exit_stub:
 *  Stub function registered by the library via atexit.
 */
static void allegro_exit_stub(void)
{
   _allegro_in_exit = TRUE;

   allegro_exit();
}



/* _install_allegro:
 *  Initialises the Allegro library, activating the system driver.
 */
static int _install_allegro(int system_id, int *errno_ptr, int (*atexit_ptr)(void (*func)(void)))
{
   RGB black_rgb = {0, 0, 0, 0};
   int i;

   if (errno_ptr)
      allegro_errno = errno_ptr;
   else
      allegro_errno = &errno;

   /* set up default palette structures */
   for (i=0; i<256; i++)
      black_palette[i] = black_rgb;


   /* lock some important variables */
   LOCK_VARIABLE(_drawing_mode);
   LOCK_VARIABLE(_drawing_pattern);
   LOCK_VARIABLE(_drawing_x_anchor);
   LOCK_VARIABLE(_drawing_y_anchor);
   LOCK_VARIABLE(_drawing_x_mask);
   LOCK_VARIABLE(_drawing_y_mask);
   LOCK_VARIABLE(_current_palette);

   /* initialise the system driver */
   usetc(allegro_error, 0);

   /* install shutdown handler */
   if (_allegro_count == 0) {
      if (atexit_ptr)
	 atexit_ptr(allegro_exit_stub);
   }

   _allegro_count++;

   TRACE(PREFIX_I "Allegro initialised (instance %d)\n", _allegro_count);
   return 0;
}



/* _install_allegro_version_check:
 *  Initialises the Allegro library, but return with an error if an
 *  incompatible version is found.
 */
int _install_allegro_version_check(int system_id, int *errno_ptr,
   int (*atexit_ptr)(void (*func)(void)), int version)
{
   int r = _install_allegro(system_id, errno_ptr, atexit_ptr);

   int build_wip = version & 255;
   int build_ver = version & ~255;

   int version_ok;

   if (r != 0) {
      /* failed */
      return r;
   }

#if ALLEGRO_SUB_VERSION & 1
   /* This is a WIP runtime, so enforce strict compatibility. */
   version_ok = version == MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION);
#else
   /* This is a stable runtime, so the runtime should be at least as new
    * as the build headers (otherwise we may get a crash, since some
    * functions may have been used which aren't available in this runtime).
    */
   version_ok = (MAKE_VERSION(ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, 0) == build_ver) &&
      (ALLEGRO_WIP_VERSION >= build_wip);
#endif

   if (!version_ok) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, 
         "The detected dynamic Allegro library (%d.%d.%d) is "
         "not compatible with this program (%d.%d.%d).",
         ALLEGRO_VERSION, ALLEGRO_SUB_VERSION, ALLEGRO_WIP_VERSION,
         build_ver >> 16, (build_ver >> 8) & 255, build_wip);
      return -1;
   }
   return 0;
}



/* allegro_exit:
 *  Closes down the Allegro system.
 */
void allegro_exit(void)
{
   while (exit_func_list) {
      void (*func)(void) = exit_func_list->funcptr;
      _remove_exit_func(func);
      (*func)();
   }

   if (_scratch_mem) {
      _AL_FREE(_scratch_mem);
      _scratch_mem = NULL;
      _scratch_mem_size = 0;
   }
}



/* allegro_message:
 *  Displays a message in whatever form the current platform requires.
 */
void allegro_message(AL_CONST char *msg, ...)
{
   char *buf = _AL_MALLOC_ATOMIC(ALLEGRO_MESSAGE_SIZE);
   char *tmp = _AL_MALLOC_ATOMIC(ALLEGRO_MESSAGE_SIZE);
   va_list ap;
   ASSERT(msg);
   va_start(ap, msg);
   uvszprintf(buf, ALLEGRO_MESSAGE_SIZE, msg, ap);
   va_end(ap);

   fputs(uconvert(buf, U_CURRENT, tmp, U_ASCII_CP, ALLEGRO_MESSAGE_SIZE), stdout);

   _AL_FREE(buf);
   _AL_FREE(tmp);
}



/* debug_exit:
 *  Closes the debugging output files.
 */
static void debug_exit(void)
{
   if (assert_file) {
      fclose(assert_file);
      assert_file = NULL;
   }

   if (trace_file) {
      fclose(trace_file);
      trace_file = NULL;
   }

   debug_assert_virgin = TRUE;
   debug_trace_virgin = TRUE;

   _remove_exit_func(debug_exit);
}



/* al_assert:
 *  Raises an assert (uses ASCII strings).
 */
void al_assert(AL_CONST char *file, int line)
{
   static int asserted = FALSE;
   int olderr = errno;
   char buf[128];
   char *s;

   if (asserted)
      return;

   /* todo, some day: use snprintf (C99) */
   sprintf(buf, "Assert failed at line %d of %s", line, file);

   if (assert_handler) {
      if (assert_handler(buf))
	 return;
   }

   if (debug_assert_virgin) {
      s = getenv("ALLEGRO_ASSERT");

      if (s)
	 assert_file = fopen(s, "w");
      else
	 assert_file = NULL;

      if (debug_trace_virgin)
	 _add_exit_func(debug_exit, "debug_exit");

      debug_assert_virgin = FALSE;
   }

   if (assert_file) {
      fprintf(assert_file, "%s\n", buf);
      fflush(assert_file);
   }
   else {
      asserted = TRUE;

      {
	 allegro_exit();
	 fprintf(stderr, "%s\n", buf);
	 abort();
      }
   }

   errno = olderr;
}



/* al_trace:
 *  Outputs a trace message (uses ASCII strings).
 */
void al_trace(AL_CONST char *msg, ...)
{
   int olderr = errno;
   char buf[512];
   char *s;

   /* todo, some day: use vsnprintf (C99) */
   va_list ap;
   va_start(ap, msg);
   vsprintf(buf, msg, ap);
   va_end(ap);

   if (_al_trace_handler) {
      if (_al_trace_handler(buf))
	 return;
   }

   if (debug_trace_virgin) {
      s = getenv("ALLEGRO_TRACE");

      if (s)
	 trace_file = fopen(s, "w");
      else
	 trace_file = fopen(LOGFILE, "w");

      if (debug_assert_virgin)
	 _add_exit_func(debug_exit, "debug_exit");

      debug_trace_virgin = FALSE;
   }

   if (trace_file) {
      fwrite(buf, sizeof(char), strlen(buf), trace_file);
      fflush(trace_file);
   }

   errno = olderr;
}



/* register_assert_handler:
 *  Installs a user handler for assert failures.
 */
void register_assert_handler(int (*handler)(AL_CONST char *msg))
{
   assert_handler = handler;
}



/* register_trace_handler:
 *  Installs a user handler for trace output.
 */
void register_trace_handler(int (*handler)(AL_CONST char *msg))
{
   _al_trace_handler = handler;
}



/* _al_malloc:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void *_al_malloc(size_t size)
{
   return malloc(size);
}



/* _al_free:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void _al_free(void *mem)
{
   free(mem);
}



/* _al_realloc:
 *  Wrapper for when a program needs to manipulate memory that has been
 *  allocated by the Allegro DLL.
 */
void *_al_realloc(void *mem, size_t size)
{
   return realloc(mem, size);
}



/* _al_strdup:
 *  Wrapper for when a program needs to duplicate a string in a way that
 *  uses any user overloaded memory allocation system in use.
 *  The result of this function _must_ be freed with _AL_FREE().
 */
char *_al_strdup(AL_CONST char *string)
{
   char *newstring = _AL_MALLOC(strlen(string) + 1);

   if (newstring)
      strcpy(newstring, string);

   return newstring;
}

