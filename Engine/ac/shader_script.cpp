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
#include "debug/debug_log.h"
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
    const String def_filename = definition_ext.IsEmpty() ? "" : Path::ReplaceExtension(filename, definition_ext);
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

// Recreates a shader from existing ScriptShaderProgram object.
// This is used to reconnect shaders after restoring a save.
static bool RecreateScriptShaderProgram(ScriptShaderProgram *sc_shader)
{
    IGraphicShader *shader = CreateShaderProgram(sc_shader->GetName(), sc_shader->GetFilename());
    if (shader)
    {
        shader->ResetConstants();
    }
    add_custom_shader(shader, sc_shader->GetID()); // NOTE: we allow to register a nullptr at the index
    return shader != nullptr;
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
        shinst = gfxDriver->CreateShaderInstance(shader, sc_shinst->GetName());

    ccRegisterManagedObject(sc_shinst, sc_shinst);
    add_shader_instance(shinst, sc_shinst->GetID()); // NOTE: we allow to register a nullptr at the index
    return sc_shinst;
}

static void SetShaderConstant(IShaderInstance *shader_inst, const char *name, uint32_t size,
                              float x, float y, float z, float w);
static void SetShaderTexture(IShaderInstance *shader_inst, uint32_t index, int sprite);

// Recreates a shader instance from existing ScriptShaderInstance object.
// This is used to reconnect shaders after restoring a save.
static bool RecreateShaderInstance(ScriptShaderInstance *sc_shinst)
{
    ScriptShaderProgram *sc_shader = sc_shinst->GetScriptShader();
    IShaderInstance *shinst = nullptr;
    if (sc_shader)
    {
        IGraphicShader *shader = get_custom_shader(sc_shader->GetID());
        if (shader)
            shinst = gfxDriver->CreateShaderInstance(shader, sc_shinst->GetName());
    }
    
    add_shader_instance(shinst, sc_shinst->GetID()); // NOTE: we allow to register a nullptr at the index
    if (!shinst)
        return false;

    // Restore constant data
    uint32_t const_count = sc_shader->GetConstantCount();
    for (uint32_t i = 0; i < const_count; ++i)
    {
        const String name = sc_shader->GetConstantName(i);
        const auto c = sc_shinst->GetConstant(i);
        SetShaderConstant(shinst, name.GetCStr(), c.Size, c.Val[0], c.Val[1], c.Val[2], c.Val[3]);
    }
    // Restore secondary textures
    for (uint32_t i = 1; i < ScriptShaderProgram::SecondaryTextureCount; ++i)
    {
        SetShaderTexture(shinst, i, sc_shinst->GetTexture(i));
    }

    return true;
}

// A callback that resolves a ScriptShaderProgram after restoring a save.
// Used in a call to ccTraverseManagedObjects.
static void ResolveShader(int handle, IScriptObject *obj)
{
    RecreateScriptShaderProgram(static_cast<ScriptShaderProgram *>(obj));
}

// A callback that resolves a ScriptShaderInstance after restoring a save.
// Used in a call to ccTraverseManagedObjects.
static void ResolveShaderInstance(int handle, IScriptObject *obj)
{
    RecreateShaderInstance(static_cast<ScriptShaderInstance *>(obj));
}

void RestoreShaders()
{
    ccTraverseManagedObjects(ScriptShaderProgram::TypeID, ResolveShader);
    ccTraverseManagedObjects(ScriptShaderInstance::TypeID, ResolveShaderInstance);
}

ScriptShaderInstance *ShaderProgram_GetDefault(ScriptShaderProgram *shader_prg)
{
    return static_cast<ScriptShaderInstance*>(shader_prg->GetDefaultInstance());
}

static void SetShaderConstant(IShaderInstance *shader_inst, const char *name, uint32_t size,
                              float x, float y, float z, float w)
{
    const uint32_t const_index = shader_inst->GetShader()->GetConstantByName(name);
    if (const_index != UINT32_MAX)
    {
        // Pass new values to the graphics shader instance
        switch (size)
        {
        case 1: shader_inst->SetShaderConstantF(const_index, x); break;
        case 2: shader_inst->SetShaderConstantF2(const_index, x, y); break;
        case 3: shader_inst->SetShaderConstantF3(const_index, x, y, z); break;
        case 4: shader_inst->SetShaderConstantF4(const_index, x, y, z, w); break;
        default: assert(false); break;
        }
    }
}

static void SetShaderTexture(IShaderInstance *shader_inst, uint32_t index, int sprite)
{
    std::shared_ptr<Texture> tex = sprite > 0 ? texturecache_precache(sprite) : nullptr;
    shader_inst->SetShaderSampler(index, tex);
}

void ShaderInstance_SetConstantFX(ScriptShaderInstance *sc_shinst, const char *name, uint32_t size,
                                  float x = 0.f, float y = 0.f, float z = 0.f, float w = 0.f)
{
    // Cache the constant and data in the script object;
    // do this regardless of whether such constant exists in the real shader or not.
    const uint32_t sc_index = sc_shinst->GetScriptShader()->SetConstant(name);
    float sc_val[4] = { x, y, z, w };
    sc_shinst->SetConstantData(sc_index, sc_val, size);

    // Now try getting the actual shader and apply the constant according
    // to their constants table
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (!shader_inst || !shader_inst->GetShader())
        return;

    SetShaderConstant(shader_inst, name, size, x, y, z, w);
}

void ShaderInstance_SetConstantF(ScriptShaderInstance *sc_shinst, const char *name, float value)
{
    ShaderInstance_SetConstantFX(sc_shinst, name, 1, value);
}

void ShaderInstance_SetConstantF2(ScriptShaderInstance *sc_shinst, const char *name, float x, float y)
{
    ShaderInstance_SetConstantFX(sc_shinst, name, 2, x, y);
}

void ShaderInstance_SetConstantF3(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z)
{
    ShaderInstance_SetConstantFX(sc_shinst, name, 3, x, y, z);
}

void ShaderInstance_SetConstantF4(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z, float w)
{
    ShaderInstance_SetConstantFX(sc_shinst, name, 4, x, y, z, w);
}

void ShaderInstance_SetTexture(ScriptShaderInstance *sc_shinst, int index, int sprite)
{
    if (index < 1 || index >= ScriptShaderProgram::SecondaryTextureCount)
    {
        debug_script_warn("ShaderInstance.SetTexture: unsupported texture index %d, valid range is 1..3", index);
        return;
    }

    // Cache the texture in the script object;
    // do this regardless of whether such sampler is supported in the real shader or not.
    sc_shinst->SetTexture(index, sprite);

    // Now try getting the actual shader and apply the constant according
    // to their constants table
    IShaderInstance *shader_inst = get_shader_instance(sc_shinst->GetID());
    if (!shader_inst || !shader_inst->GetShader())
        return;

    SetShaderTexture(shader_inst, index, sprite);
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

RuntimeScriptValue Sc_ShaderInstance_SetTexture(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptShaderInstance, ShaderInstance_SetTexture);
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
        { "ShaderInstance::SetTexture",    API_FN_PAIR(ShaderInstance_SetTexture)},
        { "ShaderInstance::get_Shader",    API_FN_PAIR(ShaderInstance_GetShader)},
    };

    ccAddExternalFunctions(shader_api);
}
