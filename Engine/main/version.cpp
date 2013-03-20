
#include "main/version.h"

namespace AGS
{
namespace Engine
{

const Version Version::LastOldFormatVersion(3, 22, 0, 1120);

Version::Version()
    : Major(0)
    , Minor(0)
    , Release(0)
    , Revision(0)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Revision(0)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release, int32_t revision)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Revision(revision)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release, int32_t revision, const String &special)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Revision(revision)
    , Special(special)
{
    MakeString();
}

Version::Version(int32_t major, int32_t minor, int32_t release, int32_t revision, const String &special, const String &build_info)
    : Major(major)
    , Minor(minor)
    , Release(release)
    , Revision(revision)
    , Special(special)
    , BuildInfo(build_info)
{
    MakeString();
}

Version::Version(const String &version_string)
    : Major(0)
    , Minor(0)
    , Release(0)
    , Revision(0)
{
    SetFromString(version_string);
}

void Version::SetFromString(const String &version_string)
{
    Major = version_string.LeftSection('.').ToInt();
    String minor_section = version_string.Section('.', 1, 2);
    Minor = minor_section.ToInt();

    if (Major <= LastOldFormatVersion.Major &&
        Minor <= LastOldFormatVersion.Minor)
    {
        if (minor_section.GetLength() == 1)
        {
            Minor *= 10;
        }
        Release = 0;
        Revision = version_string.Section('.', 2, 3).ToInt();
        Special = version_string.Section('.', 3, 4);
    }
    else
    {
        Release = version_string.Section('.', 2, 3).ToInt();
        Revision = version_string.Section('.', 3, 4).ToInt();
        Special = version_string.Section('.', 4, 5);
    }
    
    MakeString();
}

void Version::MakeString()
{
    if (Special.IsEmpty())
    {
        LongString.Format("%d.%d.%d.%d", Major, Minor, Release, Revision);
        BackwardCompatibleString.Format("%d.%d.%d", Major, Minor, Revision);
    }
    else
    {
        LongString.Format("%d.%d.%d.%d.%s", Major, Minor, Release, Revision, Special.GetCStr());
        BackwardCompatibleString.Format("%d.%d.%d%s", Major, Minor, Revision, Special.GetCStr());
    }
    ShortString.Format("%d.%d", Major, Minor);
}

} // namespace Engine
} // namespace AGS
