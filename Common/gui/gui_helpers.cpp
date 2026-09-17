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
#include "gui/gui_helpers.h"

namespace AGS
{
namespace Common
{
namespace GUI
{

void ScanMacroTokens(const String &text, std::function<bool(const String &macro)> &fn_parse,
    bool allow_unclosed_macros)
{
    for (size_t scan_at = text.FindChar('@'); scan_at != String::NoIndex;)
    {
        const size_t macro_at = scan_at + 1;
        size_t macro_end = scan_at = text.FindChar('@', scan_at + 1);
        if (macro_end == String::NoIndex)
        {
            // Malformed macro string
            if (!allow_unclosed_macros)
                break;

            macro_end = text.GetLength();
        }
        if (macro_end == macro_at)
        {
            // Zero-length substring, ignore and continue from the last '@'
            continue;
        }

        // Pass macro to callback
        // TODO: this is where a string view class would come handy
        const size_t macro_len = macro_end - macro_at;
        if (!fn_parse(text.Mid(macro_at, macro_len)))
            continue; // no matching macro, ignore and continue from the last '@'

        // If macro was resolved, then search for the next @...@ pair
        scan_at = text.FindChar('@', macro_end + 1);
    }
}

String ResolveMacroTokens(const String &text, std::function<bool(const String &macro, String &result)> &fn_parse,
    bool allow_unclosed_macros)
{
    String resolved_text;
    size_t text_at = 0u;
    for (size_t scan_at = text.FindChar('@'); scan_at != String::NoIndex;)
    {
        const size_t macro_at = scan_at + 1;
        size_t macro_end = scan_at = text.FindChar('@', scan_at + 1);
        if (macro_end == String::NoIndex)
        {
            // Malformed macro string
            if (!allow_unclosed_macros)
                break;

            macro_end = text.GetLength();
        }
        if (macro_end == macro_at)
        {
            // Zero-length substring, ignore and continue from the last '@'
            continue;
        }

        // Copy literal text (if there's any between macros)
        resolved_text.Append(text.Mid(text_at, macro_at - 1 - text_at));
        text_at = macro_at - 1;

        // Test which macro is it
        // TODO: this is where a string view class would come handy
        const size_t macro_len = macro_end - macro_at;
        String result;
        if (!fn_parse(text.Mid(macro_at, macro_len), result))
            continue; // no matching macro, continue from the last '@'

        // If macro was resolved, then append result and search for the next @...@ pair
        resolved_text.Append(result);
        text_at = macro_end + 1;
        scan_at = text.FindChar('@', macro_end + 1);
    }

    // Copy trailing literal text (if there's any)
    resolved_text.Append(text.Mid(text_at));
    return resolved_text;
}

} // namespace GUI
} // namespace Common
} // namespace AGS
