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
#include "ac/keycode.h"

eAGSKeyCode AGSKeyToScriptKey(eAGSKeyCode keycode)
{
    // Script API requires strictly capital letters, if this is a small letter - capitalize it
    return (keycode >= 'a' && keycode <= 'z') ?
        static_cast<eAGSKeyCode>(keycode - 'a' + 'A') : keycode;
}

char AGSKeyToText(eAGSKeyCode keycode)
{
    // support only printable characters (128-255 are chars from extended fonts)
    if (keycode >= 32 && keycode < 256)
        return static_cast<char>(keycode);
    return 0;
}
