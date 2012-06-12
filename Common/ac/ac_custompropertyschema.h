#ifndef __AC_CUSTOMPROPERTYSCHEMA_H
#define __AC_CUSTOMPROPERTYSCHEMA_H

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

    void Serialize (FILE *outto);
    int UnSerialize (FILE *infrom);

};

#endif // __AC_CUSTOMPROPERTYSCHEMA_H