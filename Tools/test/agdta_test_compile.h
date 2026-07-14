// Compile fixture scripts for agdta_test only (not part of agdta CLI).
#pragma once

#include "build_system.h"

namespace AGSBuild
{

class Project;

// Writes compiled *.o files to <output_base_dir>/_temp (same layout agdta expects).
bool CompileFixtureScripts(Project& project, const BuildConfig& config, BuildResult& result);

} // namespace AGSBuild
