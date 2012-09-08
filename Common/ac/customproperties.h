#ifndef __AC_CUSTOMPROPERTIES_H
#define __AC_CUSTOMPROPERTIES_H

#include "util/file.h"

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#define MAX_CUSTOM_PROPERTIES 30
#define MAX_CUSTOM_PROPERTY_VALUE_LENGTH 500
#define PROP_TYPE_BOOL   1
#define PROP_TYPE_INT    2
#define PROP_TYPE_STRING 3
struct CustomPropertySchema {
    char  propName[MAX_CUSTOM_PROPERTIES][20];
    char  propDesc[MAX_CUSTOM_PROPERTIES][100];
    char *defaultValue[MAX_CUSTOM_PROPERTIES];
    int   propType[MAX_CUSTOM_PROPERTIES];
    int   numProps;

    // Find the index of the specified property
    int findProperty (const char *pname);

    void deleteProperty (int idx);

    void resetProperty (int idx);

    CustomPropertySchema ();

    void Serialize (Common::CDataStream *out);
    int UnSerialize (Common::CDataStream *in);

};

struct CustomProperties {
    char *propName[MAX_CUSTOM_PROPERTIES];
    char *propVal[MAX_CUSTOM_PROPERTIES];
    int   numProps;

    CustomProperties();
    const char *getPropertyValue (const char *pname);

    // Find the index of the specified property
    int findProperty (const char *pname);

    void reset ();

    void addProperty (const char *newname, const char *newval);

    void Serialize (Common::CDataStream *out);
    int  UnSerialize (Common::CDataStream *in);
};

#endif // __AC_CUSTOMPROPERTIES_H