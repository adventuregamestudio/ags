
#include "acgui/ac_guilabel.h"

const char* Label_GetText_New(GUILabel *labl);
void Label_GetText(GUILabel *labl, char *buffer);
void Label_SetText(GUILabel *labl, const char *newtx);
int Label_GetColor(GUILabel *labl);
void Label_SetColor(GUILabel *labl, int colr);
int Label_GetFont(GUILabel *labl);
void Label_SetFont(GUILabel *guil, int fontnum);
void SetLabelColor(int guin,int objn, int colr);
void SetLabelText(int guin,int objn,char*newtx);
void SetLabelFont(int guin,int objn, int fontnum);