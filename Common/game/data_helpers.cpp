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
#include "game/data_helpers.h"

namespace AGS
{
namespace Common
{

String PreprocessLineForOldStyleLinebreaks(const String &line)
{
    const size_t first_at = line.FindChar('\\');
    if (first_at == String::NoIndex)
        return line; // no escaping chars, nothing to do

    std::vector<char> out;
    const char *begin_ptr = line.GetCStr();
    const char *end_ptr = line.GetCStr() + line.GetLength();
    char last_char = 0;
    for (const char *src_ptr = line.GetCStr() + first_at; src_ptr < end_ptr; ++src_ptr)
    {
        if (last_char == '\\')
        {
            // Copy accumulated portion
            const size_t old_size = out.size();
            out.resize(out.size() + (src_ptr - begin_ptr));
            std::copy(begin_ptr, src_ptr, out.begin() + old_size);
            if (*src_ptr == '[')
            {
                // add double escaping of '['
                out.push_back('\\');
                out.push_back('[');
            }
            else
            {
                out.push_back(*src_ptr);
            }
            last_char = 0; // eat escaping char
            begin_ptr = src_ptr + 1;
        }
        else
        {
            last_char = *src_ptr;
        }
    }
    // Copy final accumulated portion (if any)
    const size_t old_size = out.size();
    out.resize(out.size() + (end_ptr - begin_ptr));
    std::copy(begin_ptr, end_ptr, out.begin() + old_size);
    return String(out.data(), out.size());
}

} // namespace Common
} // namespace AGS