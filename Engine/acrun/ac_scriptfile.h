

// object-based File routine -- struct definition
#define scFileRead   1
#define scFileWrite  2
#define scFileAppend 3
extern const char *fopenModes[];

struct sc_File : ICCDynamicObject {
    FILE *handle;

    virtual int Dispose(const char *address, bool force) {
        Close();
        delete this;
        return 1;
    }

    virtual const char *GetType() {
        return "File";
    }

    virtual int Serialize(const char *address, char *buffer, int bufsize) {
        // we cannot serialize an open file, so it will get closed
        return 0;
    }

    int OpenFile(const char *filename, int mode);
    void Close();

    sc_File() {
        handle = NULL;
    }
};

