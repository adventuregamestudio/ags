
void _DisplaySpeechCore(int chid, char *displbuf);
void _DisplayThoughtCore(int chid, const char *displbuf);
void Display(char*texx, ...);
int wgetfontheight(int font);
void draw_text_window(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1);
void get_message_text (int msnum, char *buffer, char giveErr = 1);

extern block screenop;
