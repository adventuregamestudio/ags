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
#include "util/string_types.h"

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

    uint32_t GetConstantCount() const { return _constantTable.size(); }
    const String GetConstantName(uint32_t index);
    // Register a constant name, allocate data index;
    // if such constant is already known, then returns existing index
    uint32_t SetConstant(const String &name);

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    String _name;
    String _filename;
    uint32_t _id = NullShaderID;
    int _defaultInstanceHandle = 0;
    mutable ScriptShaderInstance *_defaultInstance = nullptr;

    // The constants table in a driver-agnostic form: maps a constant
    // name to the index in ScriptShaderInstance data array.
    // Has no direct correspondence to how gfx driver stores these.
    // ScriptShaderProgram caches these, and uses for serialization,
    // and when resolving shader instances after restoring a save.
    std::unordered_map<String, uint32_t> _constantTable;

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

    // Constants data array, in a generic form.
    // Has no direct correspondence to how gfx driver stores these.
    // ScriptShaderInstance caches these, and uses for serialization,
    // and when resolving shader instances after restoring a save.
    struct Constant
    {
        uint32_t Size = 0u; // size of the constant, in floats
        float Val[4] = { 0.f };
    };

    uint32_t GetConstantCount() const { return _constData.size(); }
    Constant GetConstant(uint32_t index) const { return index < _constData.size() ? _constData[index] : Constant(); }
    void SetConstantData(const String &name, float value[4], uint32_t size);
    void SetConstantData(uint32_t index, float value[4], uint32_t size);

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    String _name;
    uint32_t _id = NullInstanceID;
    int _shaderHandle = 0; // script shader handle
    mutable ScriptShaderProgram *_scriptShader = nullptr;
    std::vector<Constant> _constData;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSHADER_H
