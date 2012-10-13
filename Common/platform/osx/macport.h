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
// macport.h
// Created by Steve McCrea on 6/16/05.
//
// Interface, display and misc code for the macosx port
//
//=============================================================================

#ifndef __MACPORT_H__
#define __MACPORT_H__

// replacement for filelength(int handle)
long flength(FILE *);

struct DialogTopic;
void preprocess_dialog_script(DialogTopic *);

#endif  __MACPORT_H__
