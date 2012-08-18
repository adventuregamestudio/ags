
#include "util/wgt2allg.h"
#include "ac/dynobj/scriptdialogoptionsrendering.h"

// return the type name of the object
const char *ScriptDialogOptionsRendering::GetType() {
    return "DialogOptionsRendering";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int ScriptDialogOptionsRendering::Serialize(const char *address, char *buffer, int bufsize) {
    return 0;
}

void ScriptDialogOptionsRendering::Unserialize(int index, const char *serializedData, int dataSize) {
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptDialogOptionsRendering::Reset()
{
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    parserTextboxX = 0;
    parserTextboxY = 0;
    parserTextboxWidth = 0;
    dialogID = 0;
    surfaceToRenderTo = NULL;
    surfaceAccessed = false;
    activeOptionID = -1;
}

ScriptDialogOptionsRendering::ScriptDialogOptionsRendering()
{
    Reset();
}
