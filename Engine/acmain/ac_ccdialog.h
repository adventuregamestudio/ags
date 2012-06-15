#ifndef __AC_CCDIALOG_H
#define __AC_CCDIALOG_H

struct CCDialog : AGSCCDynamicObject {

    // return the type name of the object
    virtual const char *GetType() {
        return "Dialog";
    }

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize) {
        ScriptDialog *shh = (ScriptDialog*)address;
        StartSerialize(buffer);
        SerializeInt(shh->id);
        return EndSerialize();
    }

    virtual void Unserialize(int index, const char *serializedData, int dataSize) {
        StartUnserialize(serializedData, dataSize);
        int num = UnserializeInt();
        ccRegisterUnserializedObject(index, &scrDialog[num], this);
    }

};

#endif // __AC_CCDIALOG_H
