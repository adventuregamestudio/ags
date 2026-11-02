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
//
// ScriptAnnotations is a helper that constructs and parses script annotations;
// a counterpart of the Editor's AGS.CScript.Compiler/Entities/ScriptAnnotations.cs.
//
//=============================================================================
#ifndef __SCRIPT_ANNOTATIONS_H
#define __SCRIPT_ANNOTATIONS_H

#include "util/string.h"

namespace AGS {
namespace Preprocessor {
namespace ScriptAnnotations {

    Common::String MakeNewScriptMarker(const Common::String &scriptName);

    Common::String MakeNewScriptMarker(const Common::String &scriptName, const Common::String &moduleName);

    bool ParseNewScriptMarker(const Common::String &marker, Common::String &scriptName);

    bool ParseNewScriptMarker(const Common::String &marker, Common::String &scriptName, Common::String &moduleName);

} // ScriptAnnotations
} // Preprocessor
} // AGS

#endif // __SCRIPT_ANNOTATIONS_H
