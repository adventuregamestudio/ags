
#include "main/version.h"

namespace AGS
{
namespace Engine
{

Version::Version()
    : Major(0)
    , Minor(0)
    , Release(0)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release)
    : Major(major)
    , Minor(minor)
    , Release(release)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release, const String &special)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Special(special)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release, const String &special, const String &build_info)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Special(special)
    , BuildInfo(build_info)
{
    MakeString();
}

Version::Version(const String &version_string)
    : Major(0)
    , Minor(0)
    , Release(0)
{
    SetFromString(version_string);
}

void Version::SetFromString(const String &version_string)
{
    Major = version_string.LeftSection('.').ToInt();
    Minor = version_string.Section('.', 1, 2).ToInt();
    Release = version_string.Section('.', 2, 3).ToInt();
    Special = version_string.Section('.', 3, 4);
    MakeString();
}

void Version::MakeString()
{
    if (Special.IsEmpty())
        LongString.Format("%d.%d.%d", Major, Minor, Release);
    else
        LongString.Format("%d.%d.%d.%s", Major, Minor, Release, Special.GetCStr());
    ShortString.Format("%d.%d", Major, Minor);
}

} // namespace Engine
} // namespace AGS
