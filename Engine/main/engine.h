
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__ENGINE_H
#define __AGS_EE_MAIN__ENGINE_H

const char *get_engine_version();
void		show_preload ();
void		init_game_settings();
int         initialize_engine(int argc,char*argv[]);
int			initialize_engine_with_exception_handling(int argc,char*argv[]);

extern char *music_file;
extern char *speech_file;

#endif // __AGS_EE_MAIN__ENGINE_H
