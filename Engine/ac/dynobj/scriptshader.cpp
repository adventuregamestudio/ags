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
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// TODO: I do not like having IGraphicsDriver dependency in here,
// consider another way to order shader instance deletion?
extern IGraphicsDriver *gfxDriver;

const char *ScriptShaderProgram::TypeID = "ShaderProgram";
const char *ScriptShaderInstance::TypeID = "ShaderInstance";

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

const String ScriptShaderProgram::GetConstantName(uint32_t index)
{
    for (const auto &c : _constantTable)
        if (c.second == index)
            return c.first;
    return {};
}

uint32_t ScriptShaderProgram::SetConstant(const String &name)
{
    auto it_found = _constantTable.find(name);
    if (it_found != _constantTable.end())
        return it_found->second;

    _constantTable[name] = _constantTable.size();
    return _constantTable.size();
}

const char *ScriptShaderProgram::GetType()
{
    return TypeID;
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

void ScriptShaderProgram::Unserialize(int index, Stream *in, size_t data_sz)
{
    // Header
    in->ReadInt32(); // header size
    in->ReadInt32(); // version
    in->ReadInt32(); // reserved
    _name = StrUtil::ReadString(in);
    _filename = StrUtil::ReadString(in);
    _id = in->ReadInt32();
    _defaultInstanceHandle = in->ReadInt32();
    // Constants table
    uint32_t const_count = in->ReadInt32();
    for (uint32_t i = 0; i < const_count; ++i)
    {
        String name = StrUtil::ReadString(in);
        uint32_t index = in->ReadInt32();
        _constantTable[name] = index;
    }

    ccRegisterUnserializedObject(index, this, this);
}

size_t ScriptShaderProgram::CalcSerializeSize(const void *address)
{
    const uint32_t header_sz = _name.GetLength() + _filename.GetLength() + sizeof(uint32_t) * 2
        + sizeof(uint32_t) * 5;
    size_t constant_table_sz = sizeof(uint32_t);
    for (const auto &c : _constantTable)
    {
        constant_table_sz +=
            c.first.GetLength() + sizeof(uint32_t)
            + sizeof(uint32_t);
    }
    return header_sz + constant_table_sz
        + sizeof(uint32_t); // default instance handle
}

void ScriptShaderProgram::Serialize(const void *address, Stream *out)
{
    // Header
    const uint32_t header_sz = _name.GetLength() + _filename.GetLength() + sizeof(uint32_t) * 2
        + sizeof(uint32_t) * 5;
    out->WriteInt32(header_sz); // header size
    out->WriteInt32(0); // version
    out->WriteInt32(0); // reserved
    StrUtil::WriteString(_name, out);
    StrUtil::WriteString(_filename, out);
    out->WriteInt32(_id);
    out->WriteInt32(_defaultInstanceHandle);
    // Constants table
    out->WriteInt32(_constantTable.size());
    for (const auto &c : _constantTable)
    {
        StrUtil::WriteString(c.first, out); // constant name
        out->WriteInt32(c.second); // constant location
    }
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

void ScriptShaderInstance::SetConstantData(const String &name, float value[4], uint32_t size)
{
    if (!_scriptShader)
        return;

    uint32_t index = _scriptShader->SetConstant(name);
    SetConstantData(index, value, size);
}

void ScriptShaderInstance::SetConstantData(uint32_t index, float value[4], uint32_t size)
{
    if (_constData.size() <= index)
        _constData.resize(index + 1);
    _constData[index].Size = size;
    _constData[index].Val[0] = value[0];
    _constData[index].Val[1] = value[1];
    _constData[index].Val[2] = value[2];
    _constData[index].Val[3] = value[3];
}

const char *ScriptShaderInstance::GetType()
{
    return TypeID;
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

void ScriptShaderInstance::Unserialize(int index, Stream *in, size_t data_sz)
{
    // Header
    in->ReadInt32(); // header size
    in->ReadInt32(); // version
    in->ReadInt32(); // reserved
    _id = in->ReadInt32();
    _shaderHandle = in->ReadInt32();
    // Constant data
    size_t constdata_sz = in->ReadInt32();
    _constData.resize(constdata_sz);
    for (auto &c : _constData)
    {
        c.Size = in->ReadInt32();
        in->Read(c.Val, sizeof(float) * 4);
    }

    ccRegisterUnserializedObject(index, this, this);
}

size_t ScriptShaderInstance::CalcSerializeSize(const void *address)
{
    const uint32_t header_sz = sizeof(uint32_t) * 5;
    size_t constant_table_sz = sizeof(uint32_t)
        + _constData.size() * sizeof(uint32_t) + sizeof(float) * 4;
    return header_sz + constant_table_sz
        + sizeof(uint32_t); // shader handle
}

void ScriptShaderInstance::Serialize(const void *address, Stream *out)
{
    // Header
    const uint32_t header_sz = sizeof(uint32_t) * 5;
    out->WriteInt32(header_sz); // header size
    out->WriteInt32(0); // version
    out->WriteInt32(0); // reserved
    out->WriteInt32(_id);
    out->WriteInt32(_shaderHandle);
    // Constant data
    out->WriteInt32(_constData.size());
    for (const auto &c : _constData)
    {
        out->WriteInt32(c.Size);
        out->Write(c.Val, sizeof(float) * 4);
    }
}
