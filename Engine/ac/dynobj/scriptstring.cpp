
#include "ac/dynobj/scriptstring.h"
#include "acmain/ac_string.h"
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
