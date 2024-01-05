//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/scriptdialogoptionsrendering.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

// return the type name of the object
const char *ScriptDialogOptionsRendering::GetType() {
    return "DialogOptionsRendering";
}

size_t ScriptDialogOptionsRendering::CalcSerializeSize(const void* /*address*/)
{
    return 0;
}

void ScriptDialogOptionsRendering::Serialize(const void* /*address*/, Stream* /*out*/) {
}

void ScriptDialogOptionsRendering::Unserialize(int index, Stream* /*in*/, size_t /*data_sz*/) {
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptDialogOptionsRendering::Reset()
{
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    hasAlphaChannel = false;
    parserTextboxX = 0;
    parserTextboxY = 0;
    parserTextboxWidth = 0;
    dialogID = 0;
    surfaceToRenderTo = nullptr;
    surfaceAccessed = false;
    activeOptionID = -1;
    chosenOptionID = -1;
    needRepaint = false;
}

ScriptDialogOptionsRendering::ScriptDialogOptionsRendering()
{
    Reset();
}
