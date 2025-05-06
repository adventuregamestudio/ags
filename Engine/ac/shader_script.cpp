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
#include "ac/draw.h"
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

IGraphicShader *TryCreateShaderPrecompiled(const String &name, const String &filename, const String &def_filename)
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

    return gfxDriver->CreateShaderProgram(name, data, &def);
}

IGraphicShader *TryCreateShaderFromSource(const String &name, const String &filename, const String &def_filename)
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

    return gfxDriver->CreateShaderProgram(name, shader_src.GetCStr(), &def);
}

IGraphicShader *CreateShaderProgram(const String &name, const String &filename)
{
    // Software renderer does not support shaders, so return a dummy shader program
    if (!gfxDriver->HasAcceleratedTransform())
    {
        return nullptr;
    }

    const String precompiled_ext = gfxDriver->GetShaderPrecompiledExtension();
    const String source_ext = gfxDriver->GetShaderSourceExtension();
    const String definition_ext = gfxDriver->GetShaderDefinitionExtension();

    const String file_ext = Path::GetFileExtension(filename);
    const String def_filename = Path::ReplaceExtension(filename, definition_ext);
    if (file_ext == precompiled_ext)
    {
        return TryCreateShaderPrecompiled(name, filename, def_filename);
    }
    else if (file_ext == source_ext)
    {
        return TryCreateShaderFromSource(name, filename, def_filename);
    }
    else
    {
        IGraphicShader *shader = nullptr;
        if (!precompiled_ext.IsEmpty())
            shader = TryCreateShaderPrecompiled(name, Path::ReplaceExtension(filename, precompiled_ext), def_filename);
        if (!shader && !source_ext.IsEmpty())
            shader = TryCreateShaderFromSource(name, Path::ReplaceExtension(filename, source_ext), def_filename);
        return shader;
    }
}

ScriptShaderProgram *CreateScriptShaderProgram(const char *filename)
{
    ScriptShaderProgram *sc_shader = new ScriptShaderProgram(filename);
    IGraphicShader *shader = CreateShaderProgram(sc_shader->GetName(), filename);
    ccRegisterManagedObject(sc_shader, sc_shader);
    add_custom_shader(shader, sc_shader->GetID()); // NOTE: we allow to register a nullptr at the index
    return sc_shader;
}

ScriptShaderInstance *ShaderProgram_CreateInstance(ScriptShaderProgram *sc_shader);

ScriptShaderProgram *ShaderProgram_CreateFromFile(const char *filename)
{
    ScriptShaderProgram *shader_prg = CreateScriptShaderProgram(filename);
    if (!shader_prg)
        return nullptr;

    // Create and add default shader instance
    ScriptShaderInstance *scshader_inst = ShaderProgram_CreateInstance(shader_prg);
    if (scshader_inst)
        shader_prg->SetDefaultShaderInstance(scshader_inst);
    return shader_prg;
}

ScriptShaderInstance *ShaderProgram_CreateInstance(ScriptShaderProgram *sc_shader)
{
    ScriptShaderInstance *sc_shinst = new ScriptShaderInstance(sc_shader);

    IShaderInstance *shinst = nullptr;
    IGraphicShader *shader = get_custom_shader(sc_shader->GetID());
    if (shader)
        shinst = gfxDriver->CreateShaderInstance(shader, sc_shader->GetName());

    ccRegisterManagedObject(sc_shinst, sc_shinst);
    add_shader_instance(shinst, sc_shinst->GetID()); // NOTE: we allow to register a nullptr at the index
    return sc_shinst;
}

ScriptShaderInstance *ShaderProgram_GetDefault(ScriptShaderProgram *shader_prg)
{
    return static_cast<ScriptShaderInstance*>(shader_prg->GetDefaultInstance());
}

void ShaderInstance_SetConstantF(ScriptShaderInstance *sc_shinst, const char *name, float value)
{
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF(const_index, value);
        }
    }
}

void ShaderInstance_SetConstantF2(ScriptShaderInstance *sc_shinst, const char *name, float x, float y)
{
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF2(const_index, x, y);
        }
    }
}

void ShaderInstance_SetConstantF3(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z)
{
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF3(const_index, x, y, z);
        }
    }
}

void ShaderInstance_SetConstantF4(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z, float w)
{
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (shader_inst)
    {
        uint32_t const_index = shader_inst->GetShader()->GetShaderConstant(name);
        if (const_index != UINT32_MAX)
        {
            shader_inst->SetShaderConstantF4(const_index, x, y, z, w);
        }
    }
}

ScriptShaderProgram *ShaderInstance_GetShader(ScriptShaderInstance *sc_shinst)
{
    return static_cast<ScriptShaderProgram*>(sc_shinst->GetScriptShader());
}

RuntimeScriptValue Sc_ShaderProgram_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptShaderProgram, ShaderProgram_CreateFromFile, const char);
}

RuntimeScriptValue Sc_ShaderProgram_CreateInstance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderProgram, ScriptShaderInstance, ShaderProgram_CreateInstance);
}

RuntimeScriptValue Sc_ShaderProgram_GetDefault(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderProgram, ScriptShaderInstance, ShaderProgram_GetDefault);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT(ScriptShaderInstance, ShaderInstance_SetConstantF, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT2(ScriptShaderInstance, ShaderInstance_SetConstantF2, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF3(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT3(ScriptShaderInstance, ShaderInstance_SetConstantF3, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT4(ScriptShaderInstance, ShaderInstance_SetConstantF4, const char);
}

RuntimeScriptValue Sc_ShaderInstance_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderInstance, ScriptShaderProgram, ShaderInstance_GetShader);
}

void RegisterShaderAPI()
{
    ScFnRegister shader_api[] = {
        { "ShaderProgram::CreateFromFile", API_FN_PAIR(ShaderProgram_CreateFromFile)},
        { "ShaderProgram::CreateInstance", API_FN_PAIR(ShaderProgram_CreateInstance)},
        { "ShaderProgram::get_Default",    API_FN_PAIR(ShaderProgram_GetDefault)},
        { "ShaderInstance::SetConstantF",  API_FN_PAIR(ShaderInstance_SetConstantF)},
        { "ShaderInstance::SetConstantF2", API_FN_PAIR(ShaderInstance_SetConstantF2)},
        { "ShaderInstance::SetConstantF3", API_FN_PAIR(ShaderInstance_SetConstantF3)},
        { "ShaderInstance::SetConstantF4", API_FN_PAIR(ShaderInstance_SetConstantF4)},
        { "ShaderInstance::get_Shader",    API_FN_PAIR(ShaderInstance_GetShader)},
    };

    ccAddExternalFunctions(shader_api);
}
