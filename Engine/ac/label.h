//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__LABEL_H
#define __AGS_EE_AC__LABEL_H

#include "gui/guilabel.h"

using AGS::Common::GuiLabel;

const char* Label_GetText_New(GuiLabel *labl);
void		Label_GetText(GuiLabel *labl, char *buffer);
void		Label_SetText(GuiLabel *labl, const char *newtx);
int			Label_GetColor(GuiLabel *labl);
void		Label_SetColor(GuiLabel *labl, int colr);
int			Label_GetFont(GuiLabel *labl);
void		Label_SetFont(GuiLabel *guil, int fontnum);

#endif // __AGS_EE_AC__LABEL_H
