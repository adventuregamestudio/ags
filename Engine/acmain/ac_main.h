#ifndef __AC_MAIN_H
#define __AC_MAIN_H

void do_main_cycle(int untilwhat,int daaa);
void can_run_delayed_command();
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void show_preload () ;
void init_game_settings();
void start_game();

int do_movelist_move(short*mlnum,int*xx,int*yy);
int wait_loop_still_valid();

int initialize_engine(int argc,char*argv[]);
int initialize_engine_with_exception_handling(int argc,char*argv[]);

void precache_view(int view);


#endif // __AC_MAIN_H