// Script API version constants (values match AGS.Types ScriptAPIVersion enum).
#pragma once

#include <climits>

namespace AGSBuild
{

struct ScriptAPIVersionEntry
{
    int value;
    const char *name;
};

inline const ScriptAPIVersionEntry kScriptAPIVersions[] = {
    {         0, "v321"   },
    {         1, "v330"   },
    {         2, "v334"   },
    {         3, "v335"   },
    {         4, "v340"   },
    {         5, "v341"   },
    {         6, "v350"   },
    {         7, "v3507"  },
    {         8, "v351"   },
    {   3060000, "v360"   },
    {   3060026, "v36026" },
    {   3060100, "v361"   },
    {   3060200, "v362"   },
    {   3060300, "v363"   },
};

inline constexpr int kScriptAPIVersionCount =
    sizeof(kScriptAPIVersions) / sizeof(kScriptAPIVersions[0]);
inline constexpr int kScriptAPIHighest = INT_MAX;

} // namespace AGSBuild
