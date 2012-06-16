#ifndef __AC_CCGUI_H
#define __AC_CCGUI_H

struct CCGUI : AGSCCDynamicObject {

    // return the type name of the object
    virtual const char *GetType() {
        return "GUI";
    }

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize) {
        ScriptGUI *shh = (ScriptGUI*)address;
        StartSerialize(buffer);
        SerializeInt(shh->id);
        return EndSerialize();
    }

    virtual void Unserialize(int index, const char *serializedData, int dataSize) {
        StartUnserialize(serializedData, dataSize);
        int num = UnserializeInt();
        ccRegisterUnserializedObject(index, &scrGui[num], this);
    }
};

#endif // __AC_CCGUI_H
