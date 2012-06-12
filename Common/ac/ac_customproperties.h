#ifndef __AC_CUSTOMPROPERTIES_H
#define __AC_CUSTOMPROPERTIES_H

#include "ac_custompropertyschema.h"    // constants

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

    void Serialize (FILE *outto);
    int  UnSerialize (FILE *infrom);
};

#endif // __AC_CUSTOMPROPERTIES_H