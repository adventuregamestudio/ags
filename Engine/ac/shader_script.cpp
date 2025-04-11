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

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;

ScriptShaderProgram *ShaderProgram_CreateFromFile(const char *filename)
{
    // FIXME: this is a temporary hack, we need to design a rule of how shader
    // resources are selected depending on a gfx driver
    String final_filename = filename;
    if (gfxDriver->GetDriverID() == "D3D9" &&
        Path::GetFileExtension(filename) == "glsl")
        final_filename = Path::ReplaceExtension(filename, "fxo");

    uint32_t shader_id = 0;
    auto stream = ResolveScriptPathAndOpen(final_filename, kFile_Open, kStream_Read);
    if (stream)
    {
        // TODO: do the extension check in graphics driver / graphics factory?
        String ext = Path::GetFileExtension(final_filename);
        if (ext.CompareNoCase("glsl") == 0)
        {
            // Shader source
            String shader_src = String::FromStream(stream.get());
            if (!shader_src.IsEmpty())
            {
                shader_id = gfxDriver->CreateShaderProgram(final_filename, shader_src.GetCStr());
                if (shader_id == UINT32_MAX)
                    shader_id = 0; // assign an invalid shader
            }
        }
        else if (ext.CompareNoCase("fxo") == 0)
        {
            // Shader compiled data
            std::vector<uint8_t> data(stream->GetLength());
            stream->Read(data.data(), data.size());
            if (data.size() > 0)
            {
                shader_id = gfxDriver->CreateShaderProgram(filename, data);
                if (shader_id == UINT32_MAX)
                    shader_id = 0; // assign an invalid shader
            }
        }
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
