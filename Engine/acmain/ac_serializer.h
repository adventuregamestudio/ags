
struct AGSDeSerializer : ICCObjectReader {

    virtual void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize);
};

extern AGSDeSerializer ccUnserializer;
