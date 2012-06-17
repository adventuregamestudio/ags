#ifndef __AC_CCDIALOG_H
#define __AC_CCDIALOG_H

struct CCDialog : AGSCCDynamicObject {

    // return the type name of the object
    virtual const char *GetType();

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);

    virtual void Unserialize(int index, const char *serializedData, int dataSize);

};

#endif // __AC_CCDIALOG_H
