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
#ifndef AGS_TOOL_AGFBUILDGEN__GENERATOR_COMMON_H
#define AGS_TOOL_AGFBUILDGEN__GENERATOR_COMMON_H

#include "util/string.h"

using namespace AGS::Common;

struct GeneratorOptions {
    // String ToolsDir; // not sure how to support this yet...

    String AgsDefnsFile {}; // path to agsdefns.sh

    String ToolAgfexport {};
    String ToolAgf2dlgasc {};
    String ToolAgscc {};
    String ToolTrac {};
    String ToolCrm2ash {};
    String ToolAgspak {};

    String GameProjectDir {};
    String GameProjectFile {};
    String OutputFile {};
    String OutputDir {};
    String TempDir {};

    bool HasDialogScripts {};

    std::vector<String> HeaderFileList {};
    std::vector<String> ScriptFileList {};
    std::vector<String> RoomFileList {};
    std::vector<String> TrsFileList {};
};

#endif // AGS_TOOL_AGFBUILDGEN__GENERATOR_COMMON_H
