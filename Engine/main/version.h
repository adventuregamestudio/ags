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
#ifndef __AGS_EE_MAIN__VERSION_H
#define __AGS_EE_MAIN__VERSION_H

#include "util/string.h"

namespace AGS
{
namespace Engine
{

using Common::String;

struct Version
{
    int32_t Major;
    int32_t Minor;
    int32_t Release;
    String  Special;
    String  BuildInfo;

    String  LongString;
    String  ShortString;

    Version();
    Version(int32_t major, int32_t minor, int32_t release);
    Version(int32_t major, int32_t minor, int32_t release, const String &special);
    Version(int32_t major, int32_t minor, int32_t release, const String &special, const String &build_info);
    Version(const String &version_string);

    inline int32_t AsNumber() const
    {
        return Major * 1000000 + Minor * 10000 + Release;
    }

    inline int32_t AsSmallNumber() const
    {
        return Major * 100 + Minor;
    }

    void SetFromString(const String &version_string);

    inline bool operator < (const Version &other) const
    {
        return AsNumber() < other.AsNumber();
    }

    inline bool operator <= (const Version &other) const
    {
        return AsNumber() <= other.AsNumber();
    }

    inline bool operator > (const Version &other) const
    {
        return AsNumber() > other.AsNumber();
    }

    inline bool operator >= (const Version &other) const
    {
        return AsNumber() >= other.AsNumber();
    }

    inline bool operator == (const Version &other) const
    {
        return AsNumber() == other.AsNumber();
    }

    inline bool operator != (const Version &other) const
    {
        return AsNumber() != other.AsNumber();
    }

private:
    void MakeString();
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_MAIN__VERSION_H
