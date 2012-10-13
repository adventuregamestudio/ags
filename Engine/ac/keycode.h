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
#ifndef __AGS_EE_AC__KEYCODE_H
#define __AGS_EE_AC__KEYCODE_H

#if defined(MAC_VERSION) && !defined(IOS_VERSION)
#define EXTENDED_KEY_CODE 0x3f
#else
#define EXTENDED_KEY_CODE 0
#endif

#define AGS_KEYCODE_INSERT 382
#define AGS_KEYCODE_DELETE 383
#define AGS_KEYCODE_ALT_TAB 399
#define READKEY_CODE_ALT_TAB 0x4000

#endif // __AGS_EE_AC__KEYCODE_H
