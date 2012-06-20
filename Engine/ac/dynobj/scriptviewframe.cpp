
#include "ac/dynobj/scriptviewframe.h"

int ScriptViewFrame::Dispose(const char *address, bool force) {
    // always dispose a ViewFrame
    delete this;
    return 1;
}

const char *ScriptViewFrame::GetType() {
    return "ViewFrame";
}

int ScriptViewFrame::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(view);
    SerializeInt(loop);
    SerializeInt(frame);
    return EndSerialize();
}

void ScriptViewFrame::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    view = UnserializeInt();
    loop = UnserializeInt();
    frame = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptViewFrame::ScriptViewFrame(int p_view, int p_loop, int p_frame) {
    view = p_view;
    loop = p_loop;
    frame = p_frame;
}

ScriptViewFrame::ScriptViewFrame() {
    view = -1;
    loop = -1;
    frame = -1;
}
