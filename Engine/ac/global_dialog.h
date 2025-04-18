//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALDIALOG_H
#define __AGS_EE_AC__GLOBALDIALOG_H

void RunDialog(int tum);
int GetDialogOption (int dlg, int opt);
void SetDialogOption(int dlg, int opt, int onoroff, bool dlg_script = false);
void StopDialog();

#endif // __AGS_EE_AC__GLOBALDIALOG_H
