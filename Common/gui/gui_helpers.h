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
// Helper functions related to processing GUI text and other data.
//
//=============================================================================
#ifndef __AGS_CN_GAME__GUIHELPERS_H
#define __AGS_CN_GAME__GUIHELPERS_H

#include <functional>
#include "util/string.h"

namespace AGS
{
namespace Common
{
namespace GUI
{
    // Searches the input string for the macro placeholder surrounded by '@',
    // for each found placeholder calls the provided callback function.
    // Optionally allow the last macro to be unclosed (backwards compatibility).
    void ScanMacroTokens(const String &text, std::function<bool(const String &macro)> &fn_parse,
        bool allow_unclosed_macros = false);
    // Searches the input string for the macro placeholder surrounded by '@',
    // for each found placeholder calls the provided callback function.
    // Optionally allow the last macro to be unclosed (backwards compatibility).
    String ResolveMacroTokens(const String &text, std::function<bool(const String &macro, String &result)> &fn_parse,
        bool allow_unclosed_macros = false);
} // namespace GUI
} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__GUIHELPERS_H
