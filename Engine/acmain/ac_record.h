
#define REC_MOUSECLICK 1
#define REC_MOUSEMOVE  2
#define REC_MOUSEDOWN  3
#define REC_KBHIT      4
#define REC_GETCH      5
#define REC_KEYDOWN    6
#define REC_MOUSEWHEEL 7
#define REC_SPEECHFINISHED 8
#define REC_ENDOFFILE  0x6f

int rec_misbuttondown (int but);
int rec_iskeypressed (int keycode);
int rec_mgetbutton();
int check_mouse_wheel ();
int rec_kbhit () ;
int rec_getch () ;
void stop_recording() ;
void rec_domouse (int what);


extern char replayfile[MAX_PATH];
extern int replay_time;
extern unsigned long replay_last_second;
extern int replay_start_this_time;

extern short *recordbuffer;
extern int  recbuffersize, recsize;