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
#ifndef __AGS_EE_DYNOBJ__SCRIPTSHADER_H
#define __AGS_EE_DYNOBJ__SCRIPTSHADER_H

#include <queue>
#include <unordered_set>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/string.h"

class ScriptShaderInstance;

class ScriptShaderProgram final : public CCBasicObject
{
    using String = AGS::Common::String;
public:
    const static uint32_t NullShaderID = 0u;

    // Gets next free index for the script shader
    static uint32_t GetFreeIndex();
    // Generates a shader name from the filepath
    // TODO: move this elsewhere?
    static String MakeName(const String &filepath);

    ScriptShaderProgram() = default;
    ScriptShaderProgram(const String &filename);

    const String &GetName() const { return _name; }
    const String &GetFilename() const { return _filename; }
    uint32_t GetID() const { return _id; }
    int GetDefaultInstanceHandle() const { return _defaultInstanceHandle; }
    ScriptShaderInstance *GetDefaultInstance() const;

    void SetDefaultShaderInstance(ScriptShaderInstance *shader_inst);
    uint32_t GetFreeInstanceIndex();
    void ReleaseInstanceIndex(uint32_t shinst_id);

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    String _name;
    String _filename;
    uint32_t _id = NullShaderID;
    int _defaultInstanceHandle = 0;
    mutable ScriptShaderInstance *_defaultInstance = nullptr;

    // The records of logical indexes and names of script shader objects.
    // TODO: don't use static members, consider put this in a global object like GamePlayState?
    // This also may be desired if we let create Shader entries in the editor and load them from the game data.
    // TODO: some helper class over free index counting (see also: overlays and managed pool)
    static uint32_t _nextShaderIndex;
    static std::queue<uint32_t> _freeShaderIndexes;
    static std::unordered_set<String> _registeredNames;
    // NOTE: we are using static (shared) instance indexes to let engine keep all shader indexes in a single list
    static uint32_t _nextInstanceIndex;
    static std::queue<uint32_t> _freeInstanceIndexes;
};

class ScriptShaderInstance final : public CCBasicObject
{
    using String = AGS::Common::String;
public:
    const static uint32_t NullInstanceID = 0u;

    ScriptShaderInstance() = default;
    ScriptShaderInstance(ScriptShaderProgram *sc_shader);

    const String &GetName() const { return _name; }
    uint32_t GetID() const { return _id; }
    ScriptShaderProgram *GetScriptShader() const;

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    String _name;
    uint32_t _id = NullInstanceID;
    int _shaderHandle = 0; // script shader handle
    mutable ScriptShaderProgram *_scriptShader = nullptr;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSHADER_H
