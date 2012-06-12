#ifndef __CROOM_FUNC_CORE_H
#define __CROOM_FUNC_CORE_H

extern char *croom_h_copyright;
extern char *game_file_sig;
#define GAME_FILE_VERSION 42
extern block backups[5];

extern void update_polled_stuff_if_runtime();
//#ifdef LOADROOM_DO_POLL
//extern void update_polled_stuff();
//#else
//static void update_polled_stuff() { }
//#endif

#endif // __CROOM_FUNC_CORE_H