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

int TryCreateShaderPrecompiled(const String &filename, const String &def_filename)
{
    auto stream = ResolveScriptPathAndOpen(filename, kFile_Open, kStream_Read);
    if (!stream)
        return ScriptShaderProgram::InvalidShader;

    std::vector<uint8_t> data(stream->GetLength());
    stream->Read(data.data(), data.size());
    if (data.size() == 0)
        return ScriptShaderProgram::InvalidShader;

    ShaderDefinition def;
    if (!def_filename.IsEmpty())
        LoadShaderDefinition(def_filename, def);

    uint32_t shader_id = gfxDriver->CreateShaderProgram(filename, data, &def);
    if (shader_id > INT32_MAX)
        return ScriptShaderProgram::InvalidShader; // cast to an invalid shader id
    return static_cast<int>(shader_id);
}

int TryCreateShaderFromSource(const String &filename, const String &def_filename)
{
    auto stream = ResolveScriptPathAndOpen(filename, kFile_Open, kStream_Read);
    if (!stream)
        return ScriptShaderProgram::InvalidShader;

    String shader_src = String::FromStream(stream.get());
    if (shader_src.IsEmpty())
        return ScriptShaderProgram::InvalidShader;

    ShaderDefinition def;
    if (!def_filename.IsEmpty())
        LoadShaderDefinition(def_filename, def);

    uint32_t shader_id = gfxDriver->CreateShaderProgram(filename, shader_src.GetCStr(), &def);
    if (shader_id > INT32_MAX)
        return ScriptShaderProgram::InvalidShader; // cast to an invalid shader id
    return static_cast<int>(shader_id);
}

ScriptShaderProgram *ShaderProgram_CreateFromFile(const char *filename)
{
    // Software renderer does not support shaders, so return a dummy shader program
    if (!gfxDriver->HasAcceleratedTransform())
    {
        ScriptShaderProgram *shader_prg = new ScriptShaderProgram(filename, 0);
        ccRegisterManagedObject(shader_prg, shader_prg);
        return shader_prg;
    }

    const String precompiled_ext = gfxDriver->GetShaderPrecompiledExtension();
    const String source_ext = gfxDriver->GetShaderSourceExtension();
    const String definition_ext = gfxDriver->GetShaderDefinitionExtension();

    int shader_id = ScriptShaderProgram::InvalidShader;
    const String file_ext = Path::GetFileExtension(filename);
    const String def_filename = Path::ReplaceExtension(filename, definition_ext);
    if (file_ext == precompiled_ext)
    {
        shader_id = TryCreateShaderPrecompiled(filename, def_filename);
    }
    else if (file_ext == source_ext)
    {
        shader_id = TryCreateShaderFromSource(filename, def_filename);
    }
    else
    {
        if (!precompiled_ext.IsEmpty())
            shader_id = TryCreateShaderPrecompiled(Path::ReplaceExtension(filename, precompiled_ext), def_filename);
        if (shader_id < 0 && !source_ext.IsEmpty())
            shader_id = TryCreateShaderFromSource(Path::ReplaceExtension(filename, source_ext), def_filename);
    }

    ScriptShaderProgram *shader_prg = new ScriptShaderProgram(filename, shader_id);
    ccRegisterManagedObject(shader_prg, shader_prg);
    return shader_prg;
}

int ShaderProgram_GetShaderID(ScriptShaderProgram *shader_prg)
{
    return shader_prg->GetShaderID();
}

RuntimeScriptValue Sc_ShaderProgram_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptShaderProgram, ShaderProgram_CreateFromFile, const char);
}

RuntimeScriptValue Sc_ShaderProgram_GetShaderID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptShaderProgram, ShaderProgram_GetShaderID);
}

void RegisterShaderAPI()
{
    ScFnRegister shader_api[] = {
        { "ShaderProgram::CreateFromFile", API_FN_PAIR(ShaderProgram_CreateFromFile)},
        { "ShaderProgram::get_ShaderID",   API_FN_PAIR(ShaderProgram_GetShaderID)},
    };

    ccAddExternalFunctions(shader_api);
}
