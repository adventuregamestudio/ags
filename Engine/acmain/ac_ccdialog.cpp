
#include <stdio.h>
#include "ac_ccdialog.h"
#include "acrun/ac_scriptobject.h"
#include "wgt2allg.h"
#include "acrun/ac_runninggame.h"

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
