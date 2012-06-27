
#define REC_MOUSECLICK 1
#define REC_MOUSEMOVE  2
#define REC_MOUSEDOWN  3
#define REC_KBHIT      4
#define REC_GETCH      5
#define REC_KEYDOWN    6
#define REC_MOUSEWHEEL 7
#define REC_SPEECHFINISHED 8
#define REC_ENDOFFILE  0x6f

#include "ac/rundefines.h"

void write_record_event (int evnt, int dlen, short *dbuf);
void disable_replay_playback ();
void done_playback_event (int size);
int rec_getch ();
int rec_kbhit ();

int rec_iskeypressed (int keycode);
int rec_isSpeechFinished ();
int rec_misbuttondown (int but);
int rec_mgetbutton();
void rec_domouse (int what);
int check_mouse_wheel ();
void start_recording();
void start_replay_record ();
void scStartRecording (int keyToStop);
void stop_recording();

void start_playback();



extern char replayfile[MAX_PATH];
extern int replay_time;
extern unsigned long replay_last_second;
extern int replay_start_this_time;

extern short *recordbuffer;
extern int  recbuffersize, recsize;

extern const char *replayTempFile;

extern int mouse_z_was;
