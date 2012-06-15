#ifndef __AC_CCGUIOBJECT_H
#define __AC_CCGUIOBJECT_H

struct CCGUIObject : AGSCCDynamicObject {

    // return the type name of the object
    virtual const char *GetType() {
        return "GUIObject";
    }

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize) {
        GUIObject *guio = (GUIObject*)address;
        StartSerialize(buffer);
        SerializeInt(guio->guin);
        SerializeInt(guio->objn);
        return EndSerialize();
    }

    virtual void Unserialize(int index, const char *serializedData, int dataSize) {
        StartUnserialize(serializedData, dataSize);
        int guinum = UnserializeInt();
        int objnum = UnserializeInt();
        ccRegisterUnserializedObject(index, guis[guinum].objs[objnum], this);
    }

};

#endif // __AC_CCGUIOBJECT_H