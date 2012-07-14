
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__LABEL_H
#define __AGS_EE_AC__LABEL_H

#include "gui/guilabel.h"

const char* Label_GetText_New(GUILabel *labl);
void		Label_GetText(GUILabel *labl, char *buffer);
void		Label_SetText(GUILabel *labl, const char *newtx);
int			Label_GetColor(GUILabel *labl);
void		Label_SetColor(GUILabel *labl, int colr);
int			Label_GetFont(GUILabel *labl);
void		Label_SetFont(GUILabel *guil, int fontnum);

#endif // __AGS_EE_AC__LABEL_H
