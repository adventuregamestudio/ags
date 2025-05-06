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
#include "ac/draw.h"
#include "gfx/graphicsdriver.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// TODO: I do not like having IGraphicsDriver dependency in here,
// consider another way to order shader instance deletion?
extern IGraphicsDriver *gfxDriver;

uint32_t ScriptShaderProgram::_nextShaderIndex = ScriptShaderProgram::NullShaderID + 1u;
std::queue<uint32_t> ScriptShaderProgram::_freeShaderIndexes;
std::unordered_set<String> ScriptShaderProgram::_registeredNames;
uint32_t ScriptShaderProgram::_nextInstanceIndex = ScriptShaderInstance::NullInstanceID + 1u;
std::queue<uint32_t> ScriptShaderProgram::_freeInstanceIndexes;

ScriptShaderProgram::ScriptShaderProgram(const String &filename)
    : _filename(filename)
{
    _name = MakeName(filename);
    _id = GetFreeIndex();
}

/*static */ uint32_t ScriptShaderProgram::GetFreeIndex()
{
    if (_freeShaderIndexes.size() > 0)
    {
        uint32_t index = _freeShaderIndexes.front();
        _freeShaderIndexes.pop();
        return index;
    }
    else
    {
        return _nextShaderIndex++;
    }
}

/*static*/ String ScriptShaderProgram::MakeName(const String &filepath)
{
    String namebase = Path::GetFilenameNoExt(filepath);
    String usename = namebase;
    if (_registeredNames.count(namebase) > 0)
    {
        for (uint32_t i = 0; i < UINT32_MAX; ++i)
        {
            String tryname = String::FromFormat("%s#%u", namebase.GetCStr(), i);
            if (_registeredNames.count(tryname) == 0)
            {
                usename = tryname;
                break;
            }
        }
    }
    return usename;
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

uint32_t ScriptShaderProgram::GetFreeInstanceIndex()
{
    if (_freeInstanceIndexes.size() > 0)
    {
        uint32_t index = _freeInstanceIndexes.front();
        _freeInstanceIndexes.pop();
        return index;
    }
    else
    {
        return _nextInstanceIndex++;
    }
}

void ScriptShaderProgram::ReleaseInstanceIndex(uint32_t shinst_id)
{
    _freeInstanceIndexes.push(shinst_id);
}

const char *ScriptShaderProgram::GetType()
{
    return "ShaderProgram";
}

int ScriptShaderProgram::Dispose(void *address, bool force)
{
    if (_defaultInstanceHandle > 0)
        ccReleaseObjectReference(_defaultInstanceHandle);

    _freeShaderIndexes.push(_id);
    _registeredNames.erase(_name);
    // NOTE: we probably should not delete actual shader program here,
    // as loaded & compiled shaders may be reused later.
    delete this;
    return 1;
}

ScriptShaderInstance::ScriptShaderInstance(ScriptShaderProgram *sc_shader)
    : _scriptShader(sc_shader)
{
    _id = sc_shader->GetFreeInstanceIndex();
    _name = String::FromFormat("%s.%u", sc_shader->GetName().GetCStr(), _id);

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
    if (_scriptShader)
        _scriptShader->ReleaseInstanceIndex(_id);
    if (_shaderHandle > 0)
        ccReleaseObjectReference(_shaderHandle);

    delete_shader_instance(_id);
    delete this;
    return 1;
}
