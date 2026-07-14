// Load agsdefns.sh — the built-in AGS script API header used during compilation.
#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace AGSBuild
{

inline std::vector<std::string> BuiltInScriptDefinitionSearchPaths()
{
    std::vector<std::string> paths;
#ifdef SOURCE_DIR
    paths.push_back(std::string(SOURCE_DIR) + "/Editor/AGS.Editor/Resources/agsdefns.sh");
#endif
    paths.push_back("/usr/share/ags/agsdefns.sh");
    paths.push_back("/usr/local/share/ags/agsdefns.sh");
    paths.push_back("Editor/AGS.Editor/Resources/agsdefns.sh");
    paths.push_back("../Editor/AGS.Editor/Resources/agsdefns.sh");
    return paths;
}

inline bool LoadBuiltInScriptDefinitions(std::string &out_content, std::string *loaded_from = nullptr)
{
    for (const auto &path : BuiltInScriptDefinitionSearchPaths())
    {
        std::ifstream file(path);
        if (!file.is_open())
            continue;
        std::ostringstream ss;
        ss << file.rdbuf();
        out_content = ss.str();
        if (loaded_from)
            *loaded_from = path;
        return !out_content.empty();
    }
    return false;
}

inline std::string FormatBuiltInScriptDefinitionSearchFailure()
{
    std::string msg =
        "Could not find agsdefns.sh (built-in AGS script API header). "
        "Scripts that use engine API types cannot compile without it. "
        "Install the AGS script definitions (e.g. under /usr/share/ags/) or "
        "run from an AGS source tree. Searched:";
    for (const auto &path : BuiltInScriptDefinitionSearchPaths())
        msg += "\n  " + path;
    return msg;
}

} // namespace AGSBuild
