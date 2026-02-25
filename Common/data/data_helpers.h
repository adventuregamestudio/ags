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
// Helper functions related to reading or writing game data.
//
//=============================================================================
#ifndef __AGS_CN_GAME__DATAHELPERS_H
#define __AGS_CN_GAME__DATAHELPERS_H

#include "util/string.h"

namespace AGS
{
namespace Common
{
    // This *double escapes *an escaped '[' character(old - style linebreak,
    // which must be escaped by user if they want a literal '[' in text).
    // This is required before doing standard unescaping for this line,
    // for in such case "\[" will be treated as a unknown escape sequence,
    // while "\\[" will be converted to "\[" by merging "\\" pair.
    String PreprocessLineForOldStyleLinebreaks(const String &line);
} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__DATAHELPERS_H
