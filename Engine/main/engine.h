
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__ENGINE_H
#define __AGS_EE_MAIN__ENGINE_H

#include "util/string.h"

using namespace AGS; // FIXME later

const char *get_engine_version();
void		show_preload ();
void		init_game_settings();
int         initialize_engine(int argc,char*argv[]);
int			initialize_engine_with_exception_handling(int argc,char*argv[]);

extern Common::CString music_file;
extern Common::CString speech_file;

#endif // __AGS_EE_MAIN__ENGINE_H
