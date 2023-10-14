//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/dynobj/scriptstring.h"
#include <stdlib.h>
#include <string.h>
#include "ac/string.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;


DynObjectRef ScriptString::CreateString(const char *fromText) {
    return CreateNewScriptStringObj(fromText);
}

int ScriptString::Dispose(void* /*address*/, bool /*force*/) {
    // always dispose
    if (_text) {
        free(_text);
        _text = nullptr;
    }
    delete this;
    return 1;
}

const char *ScriptString::GetType() {
    return "String";
}

size_t ScriptString::CalcSerializeSize(const void* /*address*/)
{
    return _len + 1 + sizeof(int32_t);
}

void ScriptString::Serialize(const void* /*address*/, Stream *out) {
    const auto *cstr = _text ? _text : "";
    out->WriteInt32(_len);
    out->Write(cstr, _len + 1);
}

void ScriptString::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    _len = in->ReadInt32();
    _text = (char*)malloc(_len + 1);
    in->Read(_text, _len + 1);
    _text[_len] = 0; // for safety
    ccRegisterUnserializedObject(index, _text, this);
}

ScriptString::ScriptString(const char *text) {
    _len = strlen(text);
    _text = (char*)malloc(_len + 1);
    memcpy(_text, text, _len + 1);
}

ScriptString::ScriptString(char *text, bool take_ownership) {
    _len = strlen(text);
    if (take_ownership)
    {
        _text = text;
    }
    else
    {
        _text = (char*)malloc(_len + 1);
        memcpy(_text, text, _len + 1);
    }
}
