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
#include "ac/string.h"
#include <stdlib.h>
#include <string.h>

void* ScriptString::CreateString(const char *fromText) {
    return (void*)CreateNewScriptString(fromText);
}

int ScriptString::Dispose(const char *address, bool force) {
    // always dispose
    if (text) {
        /*    char buffer[1000];
        sprintf(buffer, "String %p deleted: '%s'", text, text);
        write_log(buffer);*/
        free(text);
    }
    delete this;
    return 1;
}

const char *ScriptString::GetType() {
    return "String";
}

int ScriptString::Serialize(const char *address, char *buffer, int bufsize) {
    if (text == NULL)
        text = "";
    StartSerialize(buffer);
    SerializeInt(strlen(text));
    strcpy(&serbuffer[bytesSoFar], text);
    bytesSoFar += strlen(text) + 1;
    return EndSerialize();
}

void ScriptString::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int textsize = UnserializeInt();
    text = (char*)malloc(textsize + 1);
    strcpy(text, &serializedData[bytesSoFar]);
    ccRegisterUnserializedObject(index, text, this);
}

ScriptString::ScriptString() {
    text = NULL;
}

ScriptString::ScriptString(const char *fromText) {
    text = (char*)malloc(strlen(fromText) + 1);
    strcpy(text, fromText);
}
