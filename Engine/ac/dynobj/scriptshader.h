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

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/string.h"

struct ScriptShaderProgram final : CCBasicObject
{
    using String = AGS::Common::String;
public:
    const static uint32_t InvalidShader = -1;

    ScriptShaderProgram(const String &name, uint32_t shader_id, uint32_t shader_inst_id);

    const String &GetName() const { return _name; }
    // FIXME make uint32_t, when the script api is fixed to store pointer instead of id
    uint32_t GetShaderID() const { return _shaderID; }
    // FIXME replace with "default instance"
    uint32_t GetShaderInstanceID() const { return _shaderInstanceID; }

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    String _name;
    uint32_t _shaderID = InvalidShader;
    uint32_t _shaderInstanceID = UINT32_MAX; // FIXME: replace with "default instance"
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSHADER_H
