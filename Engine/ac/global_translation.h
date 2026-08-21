//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
#ifndef __AGS_EE_AC__GLOBALTRANSLATION_H
#define __AGS_EE_AC__GLOBALTRANSLATION_H

// Use the provided text as a key and returns the mapped translation.
// Returns the original text back if no matching translation found.
// WARNING: get_translation returns original char* ptr if no translation is found;
// for that reason make sure that you don't pass temporary buffer there, unless
// you use returned value immediately or save it in another buffer.
const char *get_translation(const char *text);
// A backwards compatible translation of the script property values.
// For 3.6.3+ games it does NO translation and always returns original string.
// For < 3.6.3 games it acts like get_translation(), unless
// kRBO_NoTextPropertyAutoTranslate behavior switch is set by config.
const char *get_compat_prop_translation(const char *text);
int IsTranslationAvailable();
// GetTranslationName assumes a string buffer of MAX_MAXSTRLEN
int GetTranslationName(char *buffer);

#endif // __AGS_EE_AC__GLOBALTRANSLATION_H
