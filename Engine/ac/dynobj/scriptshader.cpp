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
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/dynobj_manager.h"
#include "gfx/graphicsdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// TODO: I do not like having IGraphicsDriver dependency in here,
// consider another way to order shader instance deletion?
extern IGraphicsDriver *gfxDriver;

ScriptShaderProgram::ScriptShaderProgram(const String &name, uint32_t shader_id)
    : _name(name)
    , _shaderID(shader_id)
{
}

ScriptShaderInstance *ScriptShaderProgram::GetDefaultInstance() const
{
    if (!_defaultInstance)
        _defaultInstance = static_cast<ScriptShaderInstance*>(ccGetObjectAddressFromHandle(_defaultInstanceHandle));
    return _defaultInstance;
}

void ScriptShaderProgram::SetDefaultShaderInstance(ScriptShaderInstance *shader_inst)
{
    int new_handle = ccGetObjectHandleFromAddress(shader_inst);
    // TODO: implement a RAII kind of a wrapper over managed handle that does this automatically
    if (_defaultInstanceHandle > 0 && _defaultInstanceHandle != new_handle)
        ccReleaseObjectReference(_defaultInstanceHandle);
    _defaultInstance = shader_inst;
    _defaultInstanceHandle = new_handle;
    if (_defaultInstanceHandle > 0)
        ccAddObjectReference(_defaultInstanceHandle);
}

const char *ScriptShaderProgram::GetType()
{
    return "ShaderProgram";
}

int ScriptShaderProgram::Dispose(void *address, bool force)
{
    if (_defaultInstanceHandle > 0)
        ccReleaseObjectReference(_defaultInstanceHandle);

    // NOTE: we probably should not delete shader program in gfx driver here,
    // as loaded & compiled shaders may be reused later.
    delete this;
    return 1;
}

ScriptShaderInstance::ScriptShaderInstance(ScriptShaderProgram *sc_shader, const String &name, uint32_t shader_inst_id)
    : _scriptShader(sc_shader)
    , _name(name)
    , _shaderInstID(shader_inst_id)
{
    // TODO: implement a RAII kind of a wrapper over managed handle that does this automatically
    _shaderHandle = ccGetObjectHandleFromAddress(sc_shader);
    if (_shaderHandle > 0)
        ccAddObjectReference(_shaderHandle);
}

ScriptShaderProgram *ScriptShaderInstance::GetScriptShader() const
{
    if (!_scriptShader)
        _scriptShader = static_cast<ScriptShaderProgram*>(ccGetObjectAddressFromHandle(_shaderHandle));
    return _scriptShader;
}

const char *ScriptShaderInstance::GetType()
{
    return "ShaderInstance";
}

int ScriptShaderInstance::Dispose(void *address, bool force)
{
    if (_shaderHandle > 0)
        ccReleaseObjectReference(_shaderHandle);

    gfxDriver->DeleteShaderInstance(gfxDriver->GetShaderInstance(_shaderInstID));
    delete this;
    return 1;
}
