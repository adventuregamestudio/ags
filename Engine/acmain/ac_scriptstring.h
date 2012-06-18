#ifndef __AC_SCRIPTSTRING_H
#define __AC_SCRIPTSTRING_H

struct ScriptString : AGSCCDynamicObject, ICCStringClass {
    char *text;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    virtual void* CreateString(const char *fromText);

    ScriptString();
    ScriptString(const char *fromText);
};

#endif // __AC_SCRIPTSTRING_H