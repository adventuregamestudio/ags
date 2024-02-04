//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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
#ifndef __AGS_EE_AC__GLOBALTIMER_H
#define __AGS_EE_AC__GLOBALTIMER_H

void script_SetTimer(int tnum,int timeout);
int  IsTimerExpired(int tnum);

#endif // __AGS_EE_AC__GLOBALTIMER_H
