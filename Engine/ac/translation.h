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
#ifndef __AGS_EE_AC__TRANSLATION_H
#define __AGS_EE_AC__TRANSLATION_H

#include "util/string_types.h"

using AGS::Common::String;
using AGS::Common::StringMap;

void close_translation ();
bool init_translation (const String &lang, const String &fallback_lang);
// Returns current translation name, or empty string if default translation is used
String get_translation_name();
// Returns fill path to the translation file, or empty string if default translation is used
String get_translation_path();
// Returns translation map for reading only
const StringMap &get_translation_tree();

#endif // __AGS_EE_AC__TRANSLATION_H
