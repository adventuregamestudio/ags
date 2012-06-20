
#include "ac/dynobj/scriptdatetime.h"

int ScriptDateTime::Dispose(const char *address, bool force) {
    // always dispose a DateTime
    delete this;
    return 1;
}

const char *ScriptDateTime::GetType() {
    return "DateTime";
}

int ScriptDateTime::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(year);
    SerializeInt(month);
    SerializeInt(day);
    SerializeInt(hour);
    SerializeInt(minute);
    SerializeInt(second);
    SerializeInt(rawUnixTime);
    return EndSerialize();
}

void ScriptDateTime::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    year = UnserializeInt();
    month = UnserializeInt();
    day = UnserializeInt();
    hour = UnserializeInt();
    minute = UnserializeInt();
    second = UnserializeInt();
    rawUnixTime = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDateTime::ScriptDateTime() {
    year = month = day = 0;
    hour = minute = second = 0;
    rawUnixTime = 0;
}
