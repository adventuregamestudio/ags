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
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/path_helper.h"
#include "gfx/graphicsdriver.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "util/ini_util.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;

void LoadShaderDefinition(const String &filename, ShaderDefinition &def)
{
    auto stream = ResolveScriptPathAndOpen(filename, kFile_Open, kStream_Read);
    if (!stream)
        return;

    ConfigTree tree;
    IniUtil::Read(std::move(stream), tree);
    def.CompileTarget = CfgReadString(tree, "compiler", "target");
    def.EntryPoint = CfgReadString(tree, "compiler", "entry");
    if (tree.count("constants") > 0)
    {
        const auto &constant_map = tree.at("constants");
        for (const auto &c : constant_map)
            def.Constants[c.first] = StrUtil::StringToInt(c.second);
    }
}

IGraphicShader *TryCreateShaderPrecompiled(const String &filename, const String &def_filename)
{
    auto stream = ResolveScriptPathAndOpen(filename, kFile_Open, kStream_Read);
    if (!stream)
        return nullptr;

    std::vector<uint8_t> data(stream->GetLength());
    stream->Read(data.data(), data.size());
    if (data.size() == 0)
        return nullptr;

    ShaderDefinition def;
    if (!def_filename.IsEmpty())
        LoadShaderDefinition(def_filename, def);

    return gfxDriver->CreateShaderProgram(filename, data, &def);
}

IGraphicShader *TryCreateShaderFromSource(const String &filename, const String &def_filename)
{
    auto stream = ResolveScriptPathAndOpen(filename, kFile_Open, kStream_Read);
    if (!stream)
        return nullptr;

    String shader_src = String::FromStream(stream.get());
    if (shader_src.IsEmpty())
        return nullptr;

    ShaderDefinition def;
    if (!def_filename.IsEmpty())
        LoadShaderDefinition(def_filename, def);

    return gfxDriver->CreateShaderProgram(filename, shader_src.GetCStr(), &def);
}

ScriptShaderProgram *ShaderProgram_CreateFromFile(const char *filename)
{
    // Software renderer does not support shaders, so return a dummy shader program
    if (!gfxDriver->HasAcceleratedTransform())
    {
        ScriptShaderProgram *shader_prg = new ScriptShaderProgram(filename, ScriptShaderProgram::InvalidShader, ScriptShaderProgram::InvalidShader);
        ccRegisterManagedObject(shader_prg, shader_prg);
        return shader_prg;
    }

    const String precompiled_ext = gfxDriver->GetShaderPrecompiledExtension();
    const String source_ext = gfxDriver->GetShaderSourceExtension();
    const String definition_ext = gfxDriver->GetShaderDefinitionExtension();

    IGraphicShader *shader = nullptr;
    const String file_ext = Path::GetFileExtension(filename);
    const String def_filename = Path::ReplaceExtension(filename, definition_ext);
    if (file_ext == precompiled_ext)
    {
        shader = TryCreateShaderPrecompiled(filename, def_filename);
    }
    else if (file_ext == source_ext)
    {
        shader = TryCreateShaderFromSource(filename, def_filename);
    }
    else
    {
        if (!precompiled_ext.IsEmpty())
            shader = TryCreateShaderPrecompiled(Path::ReplaceExtension(filename, precompiled_ext), def_filename);
        if (!shader && !source_ext.IsEmpty())
            shader = TryCreateShaderFromSource(Path::ReplaceExtension(filename, source_ext), def_filename);
    }

    IShaderInstance *default_inst = gfxDriver->CreateShaderInstance(shader);
    ScriptShaderProgram *shader_prg = new ScriptShaderProgram(filename, shader->GetID(), default_inst->GetID());
    ccRegisterManagedObject(shader_prg, shader_prg);
    return shader_prg;
}

int ShaderProgram_GetShaderID(ScriptShaderProgram *shader_prg)
{
    return shader_prg->GetShaderInstanceID();
}

void ShaderProgram_SetConstantF(ScriptShaderProgram *shader_prg, const char *name, float value)
{
    IShaderInstance *shader_inst = gfxDriver->GetShaderInstance(shader_prg->GetShaderInstanceID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF(const_index, value);
        }
    }
}

void ShaderProgram_SetConstantF2(ScriptShaderProgram *shader_prg, const char *name, float x, float y)
{
    IShaderInstance *shader_inst = gfxDriver->GetShaderInstance(shader_prg->GetShaderInstanceID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF2(const_index, x, y);
        }
    }
}

void ShaderProgram_SetConstantF3(ScriptShaderProgram *shader_prg, const char *name, float x, float y, float z)
{
    IShaderInstance *shader_inst = gfxDriver->GetShaderInstance(shader_prg->GetShaderInstanceID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF3(const_index, x, y, z);
        }
    }
}

void ShaderProgram_SetConstantF4(ScriptShaderProgram *shader_prg, const char *name, float x, float y, float z, float w)
{
    IShaderInstance *shader_inst = gfxDriver->GetShaderInstance(shader_prg->GetShaderInstanceID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF4(const_index, x, y, z, w);
        }
    }
}

RuntimeScriptValue Sc_ShaderProgram_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptShaderProgram, ShaderProgram_CreateFromFile, const char);
}

RuntimeScriptValue Sc_ShaderProgram_GetShaderID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptShaderProgram, ShaderProgram_GetShaderID);
}

RuntimeScriptValue Sc_ShaderProgram_SetConstantF(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT(ScriptShaderProgram, ShaderProgram_SetConstantF, const char);
}

RuntimeScriptValue Sc_ShaderProgram_SetConstantF2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT2(ScriptShaderProgram, ShaderProgram_SetConstantF2, const char);
}

RuntimeScriptValue Sc_ShaderProgram_SetConstantF3(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT3(ScriptShaderProgram, ShaderProgram_SetConstantF3, const char);
}

RuntimeScriptValue Sc_ShaderProgram_SetConstantF4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT4(ScriptShaderProgram, ShaderProgram_SetConstantF4, const char);
}

void RegisterShaderAPI()
{
    ScFnRegister shader_api[] = {
        { "ShaderProgram::CreateFromFile", API_FN_PAIR(ShaderProgram_CreateFromFile)},
        { "ShaderProgram::get_ShaderID",   API_FN_PAIR(ShaderProgram_GetShaderID)},
        { "ShaderProgram::SetConstantF",   API_FN_PAIR(ShaderProgram_SetConstantF)},
        { "ShaderProgram::SetConstantF2",  API_FN_PAIR(ShaderProgram_SetConstantF2)},
        { "ShaderProgram::SetConstantF3",  API_FN_PAIR(ShaderProgram_SetConstantF3)},
        { "ShaderProgram::SetConstantF4",  API_FN_PAIR(ShaderProgram_SetConstantF4)},
    };

    ccAddExternalFunctions(shader_api);
}
