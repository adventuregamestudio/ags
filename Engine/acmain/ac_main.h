#ifndef __AC_MAIN_H
#define __AC_MAIN_H


void do_main_cycle(int untilwhat,int daaa);
void can_run_delayed_command();
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void show_preload () ;
void init_game_settings();
void start_game();


#endif // __AC_MAIN_H