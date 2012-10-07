//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifndef __AC_CUSTOMPROPERTIES_H
#define __AC_CUSTOMPROPERTIES_H

#include "util/file.h"

namespace AGS { namespace Common { class DataStream; } }
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

    void Serialize (Common::DataStream *out);
    int UnSerialize (Common::DataStream *in);

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

    void Serialize (Common::DataStream *out);
    int  UnSerialize (Common::DataStream *in);
};

#endif // __AC_CUSTOMPROPERTIES_H