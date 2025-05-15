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
#include "scriptshader.h"

using namespace AGS::Common;

ScriptShaderProgram::ScriptShaderProgram(const String &name, int shader_id)
    : _name(name)
    , _shaderID(shader_id)
{
}

const char *ScriptShaderProgram::GetType()
{
    return "ShaderProgram";
}

int ScriptShaderProgram::Dispose(void *address, bool force)
{
    //gfxDriver->DeleteShader ... ?
    delete this;
    return 1;
}
