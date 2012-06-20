
#include "ac/dynobj/cc_dialog.h"
#include "ac/ac_dialog.h"
#include "ac/dynobj/scriptdialog.h"

extern ScriptDialog scrDialog[MAX_DIALOG];

// return the type name of the object
const char *CCDialog::GetType() {
    return "Dialog";
}

// serialize the object into BUFFER (which is BUFSIZE bytes)
// return number of bytes used
int CCDialog::Serialize(const char *address, char *buffer, int bufsize) {
    ScriptDialog *shh = (ScriptDialog*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
}

void CCDialog::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrDialog[num], this);
}
