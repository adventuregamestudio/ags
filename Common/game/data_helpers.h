//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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

#include "util/error.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

inline bool ReadAndAssertCount(Stream *in, const char *objname, uint32_t expected, HError &err)
{
    uint32_t count = in->ReadInt32();
    if (count != expected)
        err = new Error(String::FromFormat("Mismatching number of %s: read %u expected %u", objname, count, expected));
    return !err.HasError();
}

} // namespace Common
} // namespace AGS
