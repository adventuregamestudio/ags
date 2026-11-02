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
#include <string.h>
#include "script/cs_parser_common.h"
#include "preproc/script_annotations.h"

using namespace AGS::Common;

namespace AGS {
namespace Preprocessor {
namespace ScriptAnnotations {

    String MakeNewScriptMarker(const String &scriptName)
    {
        return MakeNewScriptMarker(scriptName, "");
    }

    String MakeNewScriptMarker(const String &scriptName, const String &moduleName)
    {
        String escapedScriptName = scriptName;
        escapedScriptName.Replace("\\", "\\\\");
        if (moduleName.IsEmpty())
            return String::FromFormat("%s%s\"", NEW_SCRIPT_TOKEN_PREFIX, escapedScriptName.GetCStr());
        else
            return String::FromFormat("%s%s;%s\"", NEW_SCRIPT_TOKEN_PREFIX, escapedScriptName.GetCStr(), moduleName.GetCStr());
    }

    bool ParseNewScriptMarker(const String &marker, String &scriptName)
    {
        String moduleName;
        return ParseNewScriptMarker(marker, scriptName, moduleName);
    }

    bool ParseNewScriptMarker(const String &marker, String &scriptName, String &moduleName)
    {
        if (!marker.StartsWith(NEW_SCRIPT_TOKEN_PREFIX))
        {
            scriptName = "";
            moduleName = "";
            return false;
        }

        const size_t prefixLength = strlen(NEW_SCRIPT_TOKEN_PREFIX);
        const size_t appendex_at = marker.FindChar(';', prefixLength);
        if (appendex_at != String::NoIndex)
        {
            scriptName = marker.Mid(prefixLength, appendex_at - prefixLength);
            moduleName = marker.Mid(appendex_at + 1, marker.GetLength() - (appendex_at + 1) - 1);
        }
        else
        {
            scriptName = marker.Mid(prefixLength, marker.GetLength() - prefixLength - 1);
            moduleName = "";
        }
        return true;
    }

} // ScriptAnnotations
} // Preprocessor
} // AGS
