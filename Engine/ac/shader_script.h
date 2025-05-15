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
// Shader script API.
//
//=============================================================================
#ifndef __AGS_EE_AC__SHADERSCRIPT_H
#define __AGS_EE_AC__SHADERSCRIPT_H

#include "ac/dynobj/scriptshader.h"

// Creates a new shader and registers a ScriptShaderProgram object
ScriptShaderProgram *CreateScriptShaderProgram(const char *filename);
// Creates a new shader instance and registers a ScriptShaderInstance object
ScriptShaderInstance *ShaderProgram_CreateInstance(ScriptShaderProgram *sc_shader);
// Restore shaders and shader instances after loading a save
void RestoreShaders();

#endif // __AGS_EE_AC__SHADERSCRIPT_H
