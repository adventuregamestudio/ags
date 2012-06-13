
#ifndef __AC_COMMON_H
#define __AC_COMMON_H

// quit() and update_polled_stuff_if_runtime() are the project-dependent functions,
// they are defined both in Engine.App and AGS.Native.
void quit(char *);
void update_polled_stuff_if_runtime();

extern char *croom_h_copyright;
extern char *game_file_sig;

#define GAME_FILE_VERSION 42

// [ROFLMAO] What the fuck is this? [/ROFLMAO]
// Used alot in numerous modules
extern int ff;

#endif // __AC_COMMON_H